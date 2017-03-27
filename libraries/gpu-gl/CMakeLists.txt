set(TARGET_NAME gpu-gl)
setup_hifi_library()
link_hifi_libraries(shared gl gpu)
target_link_libraries(${TARGET_NAME} pthread)
GroupSources("src")

target_opengl()
target_nsight()

if (NOT ANDROID)
  target_glew()
endif ()
