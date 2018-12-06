set(TARGET_NAME animation)
setup_hifi_library(Network Script)
link_hifi_libraries(shared graphics fbx)
include_hifi_library_headers(networking)
include_hifi_library_headers(gpu)
include_hifi_library_headers(hfm)

target_nsight()
