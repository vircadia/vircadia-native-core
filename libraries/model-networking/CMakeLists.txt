set(TARGET_NAME model-networking)
setup_hifi_library()
link_hifi_libraries(shared networking graphics fbx ktx image gl)
include_hifi_library_headers(gpu)
