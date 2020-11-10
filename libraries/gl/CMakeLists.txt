set(TARGET_NAME gl)
setup_hifi_library(Gui Widgets)
link_hifi_libraries(shared)

set(OpenGL_GL_PREFERENCE "LEGACY")
target_opengl()

