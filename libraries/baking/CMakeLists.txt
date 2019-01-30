set(TARGET_NAME baking)
setup_hifi_library(Concurrent)

link_hifi_libraries(shared graphics networking ktx image fbx)
include_hifi_library_headers(gpu)
include_hifi_library_headers(hfm)

target_draco()
