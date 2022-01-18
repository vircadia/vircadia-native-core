set(TARGET_NAME gpu-gl-common)
setup_hifi_library(Concurrent)
link_hifi_libraries(shared gl gpu shaders)
GroupSources("src")

set(OpenGL_GL_PREFERENCE "LEGACY")
target_opengl()

