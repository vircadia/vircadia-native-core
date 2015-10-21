set(TARGET_NAME procedural)
AUTOSCRIBE_SHADER_LIB(gpu model)
setup_hifi_library()
link_hifi_libraries(shared gpu model model-networking)

