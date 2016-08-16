#include "GLHelpers.h"

#include <mutex>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QRegularExpression>
#include <QtCore/QProcessEnvironment>

#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include <QtOpenGL/QGL>

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
        format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
        setGLFormatVersion(format);
        if (GLDebug::enabled()) {
            qDebug() << "Enabling debug context";
            format.setOption(QSurfaceFormat::DebugContext);
        }
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    });
    return format;
}

const QGLFormat& getDefaultGLFormat() {
    // Specify an OpenGL 3.3 format using the Core profile.
    // That is, no old-school fixed pipeline functionality
    static QGLFormat glFormat;
    static std::once_flag once;
    std::call_once(once, [] {
        setGLFormatVersion(glFormat);
        glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
        glFormat.setSampleBuffers(false);
        glFormat.setDepth(false);
        glFormat.setStencil(false);
        QGLFormat::setDefaultFormat(glFormat);
    });
    return glFormat;
}

int glVersionToInteger(QString glVersion) {
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = versionParts[0].toInt();
    int minorNumber = versionParts[1].toInt();
    return majorNumber * 100 + minorNumber * 10;
}

QJsonObject getGLContextData() {
    if (!QOpenGLContext::currentContext()) {
        return QJsonObject();
    }

    QString glVersion = QString((const char*)glGetString(GL_VERSION));
    QString glslVersion = QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
    QString glVendor = QString((const char*) glGetString(GL_VENDOR));
    QString glRenderer = QString((const char*)glGetString(GL_RENDERER));

    return QJsonObject {
        { "version", glVersion },
        { "slVersion", glslVersion },
        { "vendor", glVendor },
        { "renderer", glRenderer },
    };
}

QThread* RENDER_THREAD = nullptr;

bool isRenderThread() {
    return QThread::currentThread() == RENDER_THREAD;
}


#ifdef DEBUG
static bool enableDebugLogger = true;
#else
static const QString DEBUG_FLAG("HIFI_DEBUG_OPENGL");
static bool enableDebugLogger = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
#endif

bool GLDebug::enabled() {
    return enableDebugLogger;
}

void GLDebug::log(const QOpenGLDebugMessage & debugMessage) {
    qDebug() << debugMessage;
}

void GLDebug::setupLogger(QObject* window) {
    if (enabled()) {
        QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(window);
        logger->initialize(); // initializes in the current context, i.e. ctx
        logger->enableMessages();
        QObject::connect(logger, &QOpenGLDebugLogger::messageLogged, window, [&](const QOpenGLDebugMessage & debugMessage) {
            GLDebug::log(debugMessage);
        });
    }
}