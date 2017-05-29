set(TARGET_NAME model)
AUTOSCRIBE_SHADER_LIB(gpu model)
setup_hifi_library()
link_hifi_libraries(shared ktx gpu image)