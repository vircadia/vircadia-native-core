set(TARGET_NAME procedural)
AUTOSCRIBE_SHADER_LIB(gpu graphics)
setup_hifi_library()
link_hifi_libraries(shared gpu networking graphics model-networking ktx image)

