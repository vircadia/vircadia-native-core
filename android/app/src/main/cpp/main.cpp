#include <jni.h>

#include <android/log.h>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFileSelector>
#include <QtQuick/QQuickView>
#include <PhysicsEngine.h>


int QtMsgTypeToAndroidPriority(QtMsgType type) {
    int priority = ANDROID_LOG_UNKNOWN;
    switch (type) {
        case QtDebugMsg: priority = ANDROID_LOG_DEBUG; break;
        case QtWarningMsg: priority = ANDROID_LOG_WARN; break;
        case QtCriticalMsg: priority = ANDROID_LOG_ERROR; break;
        case QtFatalMsg: priority = ANDROID_LOG_FATAL; break;
        case QtInfoMsg: priority = ANDROID_LOG_INFO; break;
        default: break;
    }
    return priority;
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    __android_log_write(QtMsgTypeToAndroidPriority(type), "Interface", message.toStdString().c_str());
}


int main(int argc, char* argv[])
{
    qInstallMessageHandler(messageHandler);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    auto physicsEngine = new PhysicsEngine({});
    QGuiApplication app(argc,argv);
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());

    auto screen = app.primaryScreen();
    if (screen) {
        auto rect = screen->availableGeometry();
        auto size = screen->availableSize();
        auto foo = screen->availableVirtualGeometry();
        auto pixelRatio = screen->devicePixelRatio();
        qDebug() << pixelRatio;
        qDebug() << rect.width();
        qDebug() << rect.height();
    }
    QQuickView view;
    view.connect(view.engine(), &QQmlEngine::quit, &app, &QCoreApplication::quit);
    new QQmlFileSelector(view.engine(), &view);
    view.setSource(QUrl("qrc:///simple.qml"));
    if (view.status() == QQuickView::Error)
        return -1;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();
    return app.exec();
}
