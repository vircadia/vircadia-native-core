set(TARGET_NAME gpu-gl)
setup_hifi_library(Concurrent)
link_hifi_libraries(shared gl gpu gpu-gl-common shaders)
if (UNIX AND NOT VIRCADIA_THREAD_DEBUGGING)
    target_link_libraries(${TARGET_NAME} pthread)
endif(UNIX AND NOT VIRCADIA_THREAD_DEBUGGING)
GroupSources("src")

set(OpenGL_GL_PREFERENCE "GLVND")
target_opengl()

