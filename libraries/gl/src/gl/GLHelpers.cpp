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
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    });
    return format;
}

int glVersionToInteger(QString glVersion) {
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = versionParts[0].toInt();
    int minorNumber = versionParts[1].toInt();
    return (majorNumber << 16) | minorNumber;
}

QJsonObject getGLContextData() {
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
