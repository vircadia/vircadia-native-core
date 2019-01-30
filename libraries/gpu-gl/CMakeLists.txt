set(TARGET_NAME gpu-gl)
setup_hifi_library(Concurrent)
link_hifi_libraries(shared gl gpu gpu-gl-common shaders)
if (UNIX)
    target_link_libraries(${TARGET_NAME} pthread)
endif(UNIX)
GroupSources("src")
target_opengl()

