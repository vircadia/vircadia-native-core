set(TARGET_NAME gpu-gl)
setup_hifi_library()
link_hifi_libraries(shared gl gpu)
GroupSources("src")

target_glew()
target_opengl()