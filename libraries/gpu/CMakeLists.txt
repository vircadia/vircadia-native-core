set(TARGET_NAME gpu)

setup_hifi_library()
link_hifi_libraries(shared shared-gui ktx shaders)

target_nsight()
target_json()
