set(TARGET_NAME fbx)
setup_hifi_library()

link_hifi_libraries(shared model networking image)
include_hifi_library_headers(gpu image)

target_draco()
