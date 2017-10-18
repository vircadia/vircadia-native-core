set(TARGET_NAME gpu-gles)
setup_hifi_library(OpenGL)
link_hifi_libraries(shared gl gpu)
GroupSources("src")

target_opengl()
target_nsight()

if (NOT ANDROID)
  target_glew()
endif ()
