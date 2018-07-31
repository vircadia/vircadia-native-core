#include "GLHelpers.h"

#include <mutex>

#include "Config.h"

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QRegularExpression>
#include <QtCore/QProcessEnvironment>

#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

size_t evalGLFormatSwapchainPixelSize(const QSurfaceFormat& format) {
    size_t pixelSize = format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize() + format.alphaBufferSize();
    // We don't apply the length of the swap chain into this pixelSize since it is not vsible for the Process (on windows).
    // Let s keep this here remember that:
    // if (format.swapBehavior() > 0) {
    //     pixelSize *= format.swapBehavior(); // multiply the color buffer pixel size by the actual swapchain depth
    // }
    pixelSize += format.stencilBufferSize() + format.depthBufferSize();
    return pixelSize;
}

bool gl::disableGl45() {
#if defined(USE_GLES)
    return false;
#else
    static const QString DEBUG_FLAG("HIFI_DISABLE_OPENGL_45");
    static bool disableOpenGL45 = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
    return disableOpenGL45;
#endif
}

void gl::getTargetVersion(int& major, int& minor) {
#if defined(USE_GLES)
    major = 3;
    minor = 2;
#else
    major = 4;
    minor = disableGl45() ? 1 : 5;
#endif
}

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
#if defined(USE_GLES)
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
#else
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
#endif
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
        format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
        int major, minor;
        ::gl::getTargetVersion(major, minor);
        format.setMajorVersion(major);
        format.setMinorVersion(minor);
        QSurfaceFormat::setDefaultFormat(format);
    });
    return format;
}

int glVersionToInteger(QString glVersion) {
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = 0, minorNumber = 0;
    if (versionParts.size() >= 2) {
        majorNumber = versionParts[0].toInt();
        minorNumber = versionParts[1].toInt();
    }
    return (majorNumber << 16) | minorNumber;
}

QJsonObject getGLContextData() {
    static QJsonObject result;
    static std::once_flag once;
    std::call_once(once, [] {
        QString glVersion = QString((const char*)glGetString(GL_VERSION));
        QString glslVersion = QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        QString glVendor = QString((const char*) glGetString(GL_VENDOR));
        QString glRenderer = QString((const char*)glGetString(GL_RENDERER));

        result = QJsonObject {
            { "version", glVersion },
            { "sl_version", glslVersion },
            { "vendor", glVendor },
            { "renderer", glRenderer },
        };
    });
    return result;
}

QThread* RENDER_THREAD = nullptr;

bool isRenderThread() {
    return QThread::currentThread() == RENDER_THREAD;
}

#if defined(Q_OS_ANDROID)
#define USE_GLES 1
#endif

namespace gl {
    void withSavedContext(const std::function<void()>& f) {
        // Save the original GL context, because creating a QML surface will create a new context
        QOpenGLContext * savedContext = QOpenGLContext::currentContext();
        QSurface * savedSurface = savedContext ? savedContext->surface() : nullptr;
        f();
        if (savedContext) {
            savedContext->makeCurrent(savedSurface);
        }
    }

    bool checkGLError(const char* name) {
        GLenum error = glGetError();
        if (!error) {
            return false;
        } 
        switch (error) {
            case GL_INVALID_ENUM:
                qCWarning(glLogging) << "GLBackend" << name << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
                break;
            case GL_INVALID_VALUE:
                qCWarning(glLogging) << "GLBackend" << name << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
                break;
            case GL_INVALID_OPERATION:
                qCWarning(glLogging) << "GLBackend" << name << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                qCWarning(glLogging) << "GLBackend" << name << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
                break;
            case GL_OUT_OF_MEMORY:
                qCWarning(glLogging) << "GLBackend" << name << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
                break;
#if !defined(USE_GLES)
            case GL_STACK_UNDERFLOW:
                qCWarning(glLogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
                break;
            case GL_STACK_OVERFLOW:
                qCWarning(glLogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
                break;
#endif
        }
        return true;
    }


    bool checkGLErrorDebug(const char* name) {
#ifdef DEBUG
        return checkGLError(name);
#else
        Q_UNUSED(name);
        return false;
#endif
    }
}
