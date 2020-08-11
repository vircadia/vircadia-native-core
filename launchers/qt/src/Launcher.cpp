#include "Launcher.h"

#include <QResource>
#include <QFileInfo>
#include <QQmlContext>
#include <QFontDatabase>

#include "LauncherWindow.h"
#include "LauncherState.h"
#include "PathUtils.h"

Launcher::Launcher(int& argc, char**argv) : QGuiApplication(argc, argv) {
    _launcherState = std::make_shared<LauncherState>();
    QString platform;
#ifdef Q_OS_WIN
    platform = "Windows";
#elif defined(Q_OS_MACOS)
    platform = "MacOS";
#endif
    _launcherWindow = std::make_unique<LauncherWindow>();
    _launcherWindow->rootContext()->setContextProperty("LauncherState", _launcherState.get());
    _launcherWindow->rootContext()->setContextProperty("PathUtils", new PathUtils());
    _launcherWindow->rootContext()->setContextProperty("Platform", platform);
    _launcherWindow->setTitle("Vircadia");
    _launcherWindow->setFlags(Qt::FramelessWindowHint | Qt::Window);
    _launcherWindow->setLauncherStatePtr(_launcherState);

    LauncherState::declareQML();

    QFontDatabase::addApplicationFont(PathUtils::fontPath("Graphik-Regular.ttf"));
    QFontDatabase::addApplicationFont(PathUtils::fontPath("Graphik-Medium.ttf"));
    QFontDatabase::addApplicationFont(PathUtils::fontPath("Graphik-Semibold.ttf"));

    _launcherWindow->setSource(QUrl(PathUtils::resourcePath("qml/root.qml")));
    _launcherWindow->setHeight(540);
    _launcherWindow->setWidth(627);
    _launcherWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    _launcherWindow->show();
}

Launcher::~Launcher() {
}
