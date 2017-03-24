set(TARGET_NAME gpu)
autoscribe_shader_lib(gpu)
setup_hifi_library()
link_hifi_libraries(shared)

target_nsight()
