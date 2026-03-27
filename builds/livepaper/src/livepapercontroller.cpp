#include "livepapercontroller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QProcess>
#include <QCryptographicHash>
#include <QTextStream>
#include <algorithm>

namespace {
constexpr int kApplyDebounceMs = 120;
constexpr qreal kScrollFollowSpeed = 4.8;
constexpr int kGalleryRepeatCopies = 5;
constexpr int kGalleryCenterCopy = kGalleryRepeatCopies / 2;

int leadingNumber(const QString &name) {
    int i = 0;
    while (i < name.size() && name[i].isDigit()) i++;
    if (i == 0) return -1;

    bool ok = false;
    const int n = name.left(i).toInt(&ok);
    return ok ? n : -1;
}

bool imageFileCandidate(const QFileInfo &fi) {
    const QString ext = fi.suffix().toLower();
    return fi.isFile() && (ext == "png" || ext == "jpg" || ext == "jpeg" ||
                           ext == "webp" || ext == "bmp" || ext == "gif");
}

QColor rgbaFromInts(int r, int g, int b, int a) {
    return QColor(
        std::clamp(r, 0, 255),
        std::clamp(g, 0, 255),
        std::clamp(b, 0, 255),
        std::clamp(a, 0, 255));
}

QString stripInlineComment(QString value) {
    const int hashPos = value.indexOf('#');
    if (hashPos >= 0) value = value.left(hashPos);
    const int semiPos = value.indexOf(';');
    if (semiPos >= 0) value = value.left(semiPos);
    return value.trimmed();
}
}

LivepaperController::LivepaperController(const QString &wallpaperDir,
                                         QString switchCmd,
                                         QString linkPath,
                                         bool useLink,
                                         QObject *parent)
    : QObject(parent),
      m_wallpaperDir(wallpaperDir),
      m_switchCmd(std::move(switchCmd)),
      m_linkPath(std::move(linkPath)),
      m_useLink(useLink) {
    m_bg = rgbaFromInts(45, 53, 59, 255);
    m_idle = rgbaFromInts(52, 63, 68, 255);
    m_hover = rgbaFromInts(61, 72, 77, 255);
    m_border = rgbaFromInts(219, 188, 127, 255);
    m_overlay = rgbaFromInts(45, 53, 59, 220);
    m_text = rgbaFromInts(211, 198, 170, 255);

    m_applyDebounce.setSingleShot(true);
    m_applyDebounce.setInterval(kApplyDebounceMs);
    connect(&m_applyDebounce, &QTimer::timeout, this, &LivepaperController::applySelected);
    m_galleryTick.setInterval(16);
    connect(&m_galleryTick, &QTimer::timeout, this, &LivepaperController::tickGalleryScroll);
    m_galleryElapsed.start();
    m_galleryTick.start();

    loadThemeConfig();
    loadWallpapers();
}

QString LivepaperController::currentLabel() const {
    const WallpaperItem *item = m_model.itemAt(m_currentIndex);
    return item ? item->label : QString();
}

void LivepaperController::setCurrentIndex(int index) {
    if (m_items.isEmpty()) return;

    if (m_items.size() == 1) {
        const bool changed = (m_currentIndex != 0) ||
            (m_galleryTargetIndex != 0) ||
            (qAbs(m_galleryIndex) > 0.0001);
        if (m_currentIndex != 0) {
            m_currentIndex = 0;
            emit currentIndexChanged();
        }
        if (m_galleryTargetIndex != 0 || qAbs(m_galleryIndex) > 0.0001) {
            m_galleryTargetIndex = 0;
            m_galleryIndex = 0.0;
            emit galleryIndexChanged();
        }
        if (changed) m_applyDebounce.start();
        return;
    }

    int normalized = index % m_items.size();
    if (normalized < 0) normalized += m_items.size();
    if (normalized == m_currentIndex && m_galleryTargetIndex >= 0) return;

    if (m_galleryTargetIndex < 0) {
        m_galleryTargetIndex = m_items.size() + normalized;
    } else if (m_currentIndex >= 0) {
        int delta = normalized - m_currentIndex;
        if (delta == m_items.size() - 1) delta = -1;
        else if (delta == -(m_items.size() - 1)) delta = 1;
        m_galleryTargetIndex += delta;
    } else {
        m_galleryTargetIndex = m_items.size() + normalized;
    }

    if (m_galleryIndex < 0.0) {
        m_galleryIndex = m_galleryTargetIndex;
        emit galleryIndexChanged();
    }

    normalizeGalleryWindow();
    m_currentIndex = normalized;
    emit currentIndexChanged();
    m_applyDebounce.start();
}

void LivepaperController::moveLeft() {
    setCurrentIndex(m_currentIndex - 1);
}

void LivepaperController::moveRight() {
    setCurrentIndex(m_currentIndex + 1);
}

void LivepaperController::exitApp() {
    QCoreApplication::quit();
}

void LivepaperController::requestClose() {
    emit closeRequested();
}

QString LivepaperController::pathAt(int index) const {
    const WallpaperItem *item = m_model.itemAt(index);
    return item ? item->path : QString();
}

QString LivepaperController::thumbPathAt(int index) const {
    const WallpaperItem *item = m_model.itemAt(index);
    if (!item) return QString();
    return item->thumbPath.isEmpty() ? item->path : item->thumbPath;
}

QString LivepaperController::labelAt(int index) const {
    const WallpaperItem *item = m_model.itemAt(index);
    return item ? item->label : QString();
}

void LivepaperController::applySelected() {
    const WallpaperItem *item = m_model.itemAt(m_currentIndex);
    if (!item) return;

    if (m_useLink && !m_linkPath.isEmpty()) {
        updateLink(item->path);
    }
    runSwitchCommand(item->path);
}

void LivepaperController::loadThemeConfig() {
    const QString path = QDir::homePath() + "/.config/hellpaper/hellpaper.conf";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) continue;

        const int eq = line.indexOf('=');
        if (eq <= 0) continue;

        const QString key = line.left(eq).trimmed();
        const QString val = stripInlineComment(line.mid(eq + 1).trimmed());

        const QStringList parts = val.split(',', Qt::SkipEmptyParts);
        if (parts.size() == 4) {
            bool ok = true;
            int rgba[4] = {0, 0, 0, 255};
            for (int i = 0; i < 4; i++) {
                bool partOk = false;
                rgba[i] = parts[i].trimmed().toInt(&partOk);
                ok = ok && partOk;
            }
            if (!ok) continue;

            const QColor c = rgbaFromInts(rgba[0], rgba[1], rgba[2], rgba[3]);
            if (key == "bg") m_bg = c;
            else if (key == "idle") m_idle = c;
            else if (key == "hover") m_hover = c;
            else if (key == "border") m_border = c;
            else if (key == "overlay") m_overlay = c;
            else if (key == "text") m_text = c;
        }
    }

    m_borderRadius = 0;
    emit themeChanged();
}

void LivepaperController::loadWallpapers() {
    QDir dir(m_wallpaperDir);
    if (!dir.exists()) return;

    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QVector<WallpaperItem> items;
    items.reserve(files.size());

    for (const QFileInfo &fi : files) {
        if (!imageFileCandidate(fi)) continue;
        if (items.size() >= m_maxWallpapers) break;
        WallpaperItem item;
        item.path = fi.absoluteFilePath();
        item.thumbPath = ensureCachedThumb(item.path);
        item.fileName = fi.fileName();
        item.label = formatDisplayName(item.fileName);
        items.push_back(item);
    }

    std::sort(items.begin(), items.end(), [](const WallpaperItem &a, const WallpaperItem &b) {
        const int na = leadingNumber(a.fileName);
        const int nb = leadingNumber(b.fileName);
        if (na >= 0 && nb >= 0 && na != nb) return na < nb;
        if (na >= 0 && nb < 0) return true;
        if (na < 0 && nb >= 0) return false;
        return a.fileName.toLower() < b.fileName.toLower();
    });

    m_items = std::move(items);
    m_model.setItems(m_items);
    emit countChanged();

    if (m_items.isEmpty()) {
        m_currentIndex = -1;
        m_galleryTargetIndex = -1;
        m_galleryIndex = -1.0;
        emit currentIndexChanged();
        emit galleryIndexChanged();
        return;
    }

    const int fromLink = findCurrentIndexFromLink();
    m_currentIndex = (fromLink >= 0) ? fromLink : 0;
    m_galleryTargetIndex = centeredGalleryIndex(m_currentIndex);
    m_galleryIndex = m_galleryTargetIndex;
    emit currentIndexChanged();
    emit galleryIndexChanged();
    m_applyDebounce.start();
}

void LivepaperController::tickGalleryScroll() {
    if (m_galleryTargetIndex < 0 || m_galleryIndex < 0.0 || m_items.isEmpty()) return;

    qint64 elapsedMs = m_galleryElapsed.restart();
    if (elapsedMs <= 0) elapsedMs = 16;
    const qreal dt = static_cast<qreal>(elapsedMs) / 1000.0;
    const qreal alpha = std::clamp(dt * kScrollFollowSpeed, 0.0, 1.0);
    const qreal next = m_galleryIndex + (static_cast<qreal>(m_galleryTargetIndex) - m_galleryIndex) * alpha;

    if (qAbs(next - m_galleryIndex) > 0.0001) {
        m_galleryIndex = next;
        emit galleryIndexChanged();
    }

    if (qAbs(m_galleryIndex - static_cast<qreal>(m_galleryTargetIndex)) < 0.001) {
        m_galleryIndex = m_galleryTargetIndex;
        emit galleryIndexChanged();
        recenterGallery();
    }
}

int LivepaperController::centeredGalleryIndex(int actualIndex) const {
    if (m_items.isEmpty()) return -1;
    if (m_items.size() == 1) return 0;
    return m_items.size() * kGalleryCenterCopy + actualIndex;
}

void LivepaperController::normalizeGalleryWindow() {
    if (m_items.size() <= 1 || m_galleryTargetIndex < 0 || m_galleryIndex < 0.0) return;

    const int count = m_items.size();
    const int minIndex = count;
    const int maxIndex = count * (kGalleryRepeatCopies - 1);
    int shift = 0;

    while (m_galleryTargetIndex + shift < minIndex) shift += count;
    while (m_galleryTargetIndex + shift >= maxIndex) shift -= count;

    if (shift == 0) return;

    m_galleryTargetIndex += shift;
    m_galleryIndex += static_cast<qreal>(shift);
    emit galleryIndexChanged();
}

void LivepaperController::recenterGallery() {
    if (m_items.size() <= 1 || m_currentIndex < 0) return;

    const int centered = centeredGalleryIndex(m_currentIndex);
    if (m_galleryTargetIndex == centered && qAbs(m_galleryIndex - static_cast<qreal>(centered)) < 0.0001) {
        return;
    }

    m_galleryTargetIndex = centered;
    m_galleryIndex = centered;
    emit galleryIndexChanged();
}

QString LivepaperController::formatDisplayName(QString fileName) {
    const int dot = fileName.lastIndexOf('.');
    if (dot > 0) fileName = fileName.left(dot);

    int i = 0;
    while (i < fileName.size() && fileName[i].isDigit()) i++;
    if (i < fileName.size() && fileName[i] == '-') i++;

    QString out = fileName.mid(i);
    out.replace('-', ' ');
    return out;
}

int LivepaperController::findCurrentIndexFromLink() const {
    if (!m_useLink || m_linkPath.isEmpty()) return -1;

    QFileInfo linkInfo(m_linkPath);
    if (!linkInfo.exists() && !linkInfo.isSymLink()) return -1;

    const QString resolved = linkInfo.symLinkTarget().isEmpty()
        ? linkInfo.canonicalFilePath()
        : QFileInfo(linkInfo.symLinkTarget()).canonicalFilePath();

    if (resolved.isEmpty()) return -1;

    for (int i = 0; i < m_items.size(); i++) {
        if (QFileInfo(m_items[i].path).canonicalFilePath() == resolved) return i;
    }
    return -1;
}

void LivepaperController::updateLink(const QString &targetPath) {
    QFileInfo linkInfo(m_linkPath);
    QDir().mkpath(linkInfo.absolutePath());

    if (QFileInfo::exists(m_linkPath) || QFileInfo(m_linkPath).isSymLink()) {
        QFile::remove(m_linkPath);
    }
    QFile::link(targetPath, m_linkPath);
}

void LivepaperController::runSwitchCommand(const QString &targetPath) {
    if (m_switchCmd.isEmpty()) return;

    QString cmd = m_switchCmd;
    const QString escaped = shellEscapeSingleQuotes(targetPath);
    cmd.replace("{}", "'" + escaped + "'");

    QProcess::startDetached("/bin/sh", QStringList() << "-lc" << cmd);
}

QString LivepaperController::shellEscapeSingleQuotes(const QString &value) {
    QString out = value;
    out.replace('\'', "'\\''");
    return out;
}

QString LivepaperController::ensureCachedThumb(const QString &sourcePath) const {
    constexpr int kThumbSize = 148;

    QFileInfo fi(sourcePath);
    if (!fi.exists()) return sourcePath;

    const QString key = QString("%1|%2|%3|%4")
        .arg(sourcePath)
        .arg(fi.size())
        .arg(fi.lastModified().toMSecsSinceEpoch())
        .arg(kThumbSize);
    const QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();

    const QString cacheDir = QDir::homePath() + "/.cache/livepaper/thumbs";
    QDir().mkpath(cacheDir);
    const QString cachePath = cacheDir + "/" + QString::fromLatin1(hash) + ".png";
    if (QFileInfo::exists(cachePath)) return cachePath;

    QImageReader reader(sourcePath);
    reader.setAutoTransform(true);
    QImage src = reader.read();
    if (src.isNull()) return sourcePath;

    const int side = std::min(src.width(), src.height());
    const int x = (src.width() - side) / 2;
    const int y = (src.height() - side) / 2;
    QImage square = src.copy(x, y, side, side);
    QImage thumb = square.scaled(kThumbSize, kThumbSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (!thumb.save(cachePath, "PNG")) return sourcePath;
    return cachePath;
}
