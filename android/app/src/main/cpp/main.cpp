#include <jni.h>

#include <android/log.h>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFileSelector>
#include <QtQuick/QQuickView>
#include <QtGui/QOpenGLContext>
#include <QtCore/QLoggingCategory>
#include <QtWebView/QtWebView>

#include <gl/Config.h>
#include <gl/OffscreenGLCanvas.h>
#include <gl/GLWindow.h>

#include <ui/OffscreenQmlSurface.h>

Q_LOGGING_CATEGORY(gpugllogging, "hifi.gl")

bool checkGLError(const char* name) {
    GLenum error = glGetError();
    if (!error) {
        return false;
    } else {
        switch (error) {
            case GL_INVALID_ENUM:
                qCDebug(gpugllogging) << "GLBackend::" << name << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
                break;
            case GL_INVALID_VALUE:
                qCDebug(gpugllogging) << "GLBackend" << name << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
                break;
            case GL_INVALID_OPERATION:
                qCDebug(gpugllogging) << "GLBackend" << name << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                qCDebug(gpugllogging) << "GLBackend" << name << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
                break;
            case GL_OUT_OF_MEMORY:
                qCDebug(gpugllogging) << "GLBackend" << name << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
                break;
            default:
                qCDebug(gpugllogging) << "GLBackend" << name << ": Unknown error: " << error;
                break;
        }
        return true;
    }
}

bool checkGLErrorDebug(const char* name) {
    return checkGLError(name);
}


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

void qt_gl_set_global_share_context(QOpenGLContext *context);

int main(int argc, char* argv[])
{
    qInstallMessageHandler(messageHandler);
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QGuiApplication app(argc,argv);
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());
    QtWebView::initialize();
    qputenv("QSG_RENDERER_DEBUG", (QStringList() << "render" << "build" << "change" << "upload" << "roots" << "dump").join(';').toUtf8());

    OffscreenGLCanvas sharedCanvas;
    if (!sharedCanvas.create()) {
        qFatal("Unable to create primary offscreen context");
    }
    qt_gl_set_global_share_context(sharedCanvas.getContext());
    auto globalContext = QOpenGLContext::globalShareContext();

    GLWindow window;
    window.create();
    window.setGeometry(qApp->primaryScreen()->availableGeometry());
    window.createContext(globalContext);
    if (!window.makeCurrent()) {
        qFatal("Unable to make primary window GL context current");
    }

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);

    static const ivec2 offscreenSize { 640, 480 };

    OffscreenQmlSurface::setSharedContext(sharedCanvas.getContext());
    OffscreenQmlSurface* qmlSurface = new OffscreenQmlSurface();
    qmlSurface->create();
    qmlSurface->resize(fromGlm(offscreenSize));
    qmlSurface->load("qrc:///simple.qml");
    qmlSurface->resume();

    auto discardLambda = qmlSurface->getDiscardLambda();

    window.showFullScreen();
    QTimer timer;
    timer.setInterval(10);
    timer.setSingleShot(false);
    OffscreenQmlSurface::TextureAndFence currentTextureAndFence;
    timer.connect(&timer, &QTimer::timeout, &app, [&]{
        window.makeCurrent();
        glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        OffscreenQmlSurface::TextureAndFence newTextureAndFence;
        if (qmlSurface->fetchTexture(newTextureAndFence)) {
            if (currentTextureAndFence.first) {
                discardLambda(currentTextureAndFence.first, currentTextureAndFence.second);
            }
            currentTextureAndFence = newTextureAndFence;
        }
        checkGLErrorDebug(__FUNCTION__);

        if (currentTextureAndFence.second) {
            glWaitSync((GLsync)currentTextureAndFence.second, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync((GLsync)currentTextureAndFence.second);
            currentTextureAndFence.second = nullptr;
        }

        if (currentTextureAndFence.first) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, currentTextureAndFence.first, 0);
            glBlitFramebuffer(0, 0, offscreenSize.x, offscreenSize.y, 100, 100, offscreenSize.x + 100, offscreenSize.y + 100, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }

        window.swapBuffers();
        window.doneCurrent();
    });
    timer.start();
    return app.exec();
}
