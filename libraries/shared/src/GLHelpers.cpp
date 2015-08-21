#include "GLHelpers.h"


QSurfaceFormat getDefaultOpenGlSurfaceFormat() {
   QSurfaceFormat format;
   // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
   format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
   format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
   format.setVersion(4, 1);
#ifdef DEBUG
   format.setOption(QSurfaceFormat::DebugContext);
#endif
   format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
   return format;
}
