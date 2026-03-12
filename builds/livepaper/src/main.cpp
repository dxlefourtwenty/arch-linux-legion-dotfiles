#include <QGuiApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QString>
#include <QMetaObject>
#include <LayerShellQt/window.h>
#include <csignal>

namespace {
QObject *g_root = nullptr;

QScreen *findScreenByName(const QString &name) {
    if (name.isEmpty()) return nullptr;

    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen && screen->name() == name) return screen;
    }
    return nullptr;
}

QString configuredOutputName() {
    const QString path =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + "/.config/dashboard/config.json";

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return QString();

    return doc.object().value("outputName").toString().trimmed();
}

void onSigUsr1(int) {
    if (!g_root) return;
    QMetaObject::invokeMethod(g_root, "startClose", Qt::QueuedConnection);
}
}

#include "livepapercontroller.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("livepaper");

    QLockFile instanceLock(QDir::tempPath() + "/livepaper.lock");
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock(0)) return 0;

    QCommandLineParser parser;
    parser.setApplicationDescription("Bottom-row live wallpaper picker");
    parser.addHelpOption();

    QCommandLineOption switchCmdOpt(
        QStringList() << "switch-cmd",
        "command template; use {} for selected image path",
        "cmd");
    QCommandLineOption linkOpt(
        QStringList() << "link",
        "symlink updated to selected wallpaper",
        "path");
    QCommandLineOption noLinkOpt(QStringList() << "no-link", "skip symlink updates");
    QCommandLineOption outputOpt(
        QStringList() << "output",
        "screen/output name to open on",
        "name");

    parser.addOption(switchCmdOpt);
    parser.addOption(linkOpt);
    parser.addOption(noLinkOpt);
    parser.addOption(outputOpt);
    parser.addPositionalArgument("dir", "wallpaper directory");

    parser.process(app);

    const QString defaultDir = QDir::homePath() + "/.config/themes/current/wallpapers";
    const QString wallpaperDir = parser.positionalArguments().isEmpty()
        ? defaultDir
        : parser.positionalArguments().first();

    const QString defaultCmd = "swww img --transition-type=grow --transition-duration=1.6 {}";
    const QString envCmd = qEnvironmentVariable("LIVEPAPER_SWITCH_CMD");
    QString switchCmd = !envCmd.isEmpty() ? envCmd : defaultCmd;
    if (parser.isSet(switchCmdOpt)) switchCmd = parser.value(switchCmdOpt);

    const QString defaultLink = QDir::homePath() + "/.config/themes/current/wallpaper.png";
    QString linkPath = parser.isSet(linkOpt) ? parser.value(linkOpt) : defaultLink;
    const bool useLink = !parser.isSet(noLinkOpt);
    const QString configuredOutput = configuredOutputName();
    const QString targetOutput = parser.isSet(outputOpt)
        ? parser.value(outputOpt).trimmed()
        : (!configuredOutput.isEmpty() ? configuredOutput : QStringLiteral("HDMI-A-1"));

    LivepaperController controller(wallpaperDir, switchCmd, linkPath, useLink);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Livepaper", &controller);
    engine.loadFromModule("LivePaper", "Main");

    if (engine.rootObjects().isEmpty()) return -1;
    g_root = engine.rootObjects().first();
    if (auto *window = qobject_cast<QQuickWindow *>(g_root)) {
        if (QScreen *screen = findScreenByName(targetOutput)) {
            window->setScreen(screen);
            if (auto *layerShellWindow = LayerShellQt::Window::get(window)) {
                layerShellWindow->setScreen(screen);
            }
        } else if (QGuiApplication::primaryScreen()) {
            QScreen *screen = QGuiApplication::primaryScreen();
            window->setScreen(screen);
            if (auto *layerShellWindow = LayerShellQt::Window::get(window)) {
                layerShellWindow->setScreen(screen);
            }
        }
        window->show();
        window->requestActivate();
    }
    std::signal(SIGUSR1, onSigUsr1);

    return app.exec();
}
