#ifndef LIVEPAPERCONTROLLER_H
#define LIVEPAPERCONTROLLER_H

#include <QObject>
#include <QColor>
#include <QTimer>
#include <QString>
#include <QElapsedTimer>

#include "wallpapermodel.h"

class LivepaperController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *model READ model CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentLabel READ currentLabel NOTIFY currentIndexChanged)
    Q_PROPERTY(qreal galleryIndex READ galleryIndex NOTIFY galleryIndexChanged)

    Q_PROPERTY(QColor bg READ bg NOTIFY themeChanged)
    Q_PROPERTY(QColor idle READ idle NOTIFY themeChanged)
    Q_PROPERTY(QColor hover READ hover NOTIFY themeChanged)
    Q_PROPERTY(QColor border READ border NOTIFY themeChanged)
    Q_PROPERTY(QColor overlay READ overlay NOTIFY themeChanged)
    Q_PROPERTY(QColor text READ text NOTIFY themeChanged)
    Q_PROPERTY(int borderRadius READ borderRadius NOTIFY themeChanged)

public:
    explicit LivepaperController(const QString &wallpaperDir,
                                 QString switchCmd,
                                 QString linkPath,
                                 bool useLink,
                                 QObject *parent = nullptr);

    QObject *model() { return &m_model; }
    int count() const { return m_items.size(); }
    int currentIndex() const { return m_currentIndex; }
    QString currentLabel() const;
    qreal galleryIndex() const { return m_galleryIndex; }

    QColor bg() const { return m_bg; }
    QColor idle() const { return m_idle; }
    QColor hover() const { return m_hover; }
    QColor border() const { return m_border; }
    QColor overlay() const { return m_overlay; }
    QColor text() const { return m_text; }
    int borderRadius() const { return m_borderRadius; }

    Q_INVOKABLE void setCurrentIndex(int index);
    Q_INVOKABLE void moveLeft();
    Q_INVOKABLE void moveRight();
    Q_INVOKABLE void exitApp();
    Q_INVOKABLE void requestClose();
    Q_INVOKABLE QString pathAt(int index) const;
    Q_INVOKABLE QString thumbPathAt(int index) const;
    Q_INVOKABLE QString labelAt(int index) const;

signals:
    void currentIndexChanged();
    void countChanged();
    void themeChanged();
    void closeRequested();
    void galleryIndexChanged();

private slots:
    void applySelected();
    void tickGalleryScroll();

private:
    void loadThemeConfig();
    void loadWallpapers();
    int centeredGalleryIndex(int actualIndex) const;
    void normalizeGalleryWindow();
    void recenterGallery();
    static QString formatDisplayName(QString fileName);
    int findCurrentIndexFromLink() const;
    void updateLink(const QString &targetPath);
    void runSwitchCommand(const QString &targetPath);
    static QString shellEscapeSingleQuotes(const QString &value);
    QString ensureCachedThumb(const QString &sourcePath) const;

    WallpaperModel m_model;
    QVector<WallpaperItem> m_items;
    QString m_wallpaperDir;
    QString m_switchCmd;
    QString m_linkPath;
    bool m_useLink = true;
    int m_currentIndex = -1;
    int m_galleryTargetIndex = -1;
    qreal m_galleryIndex = -1.0;

    QColor m_bg;
    QColor m_idle;
    QColor m_hover;
    QColor m_border;
    QColor m_overlay;
    QColor m_text;
    int m_borderRadius = 0;
    int m_maxWallpapers = 1024;

    QTimer m_applyDebounce;
    QTimer m_galleryTick;
    QElapsedTimer m_galleryElapsed;
};

#endif
