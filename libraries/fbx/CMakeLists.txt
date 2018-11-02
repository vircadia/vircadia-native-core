set(TARGET_NAME fbx)
setup_hifi_library()

link_hifi_libraries(shared graphics networking image)
include_hifi_library_headers(gpu image)
include_hifi_library_headers(hfm)

target_draco()
