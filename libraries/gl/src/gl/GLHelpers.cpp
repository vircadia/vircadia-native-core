#include "GLHelpers.h"

#include <mutex>

#include <QtGui/QSurfaceFormat>
#include <QtOpenGL/QGL>

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
        format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
        format.setVersion(4, 5);
#ifdef DEBUG
        format.setOption(QSurfaceFormat::DebugContext);
#endif
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    });
    return format;
}

const QGLFormat& getDefaultGLFormat() {
    // Specify an OpenGL 3.3 format using the Core profile.
    // That is, no old-school fixed pipeline functionality
    static QGLFormat glFormat;
    static std::once_flag once;
    std::call_once(once, [] {
        glFormat.setVersion(4, 5);
        glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
        glFormat.setSampleBuffers(false);
        glFormat.setDepth(false);
        glFormat.setStencil(false);
    });
    return glFormat;
}
