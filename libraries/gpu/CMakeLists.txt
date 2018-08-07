set(TARGET_NAME gpu)

setup_hifi_library()
link_hifi_libraries(shared ktx shaders)

target_nsight()
