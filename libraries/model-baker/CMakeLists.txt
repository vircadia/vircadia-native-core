set(TARGET_NAME model-baker)
setup_hifi_library()

link_hifi_libraries(shared task)

include_hifi_library_headers(gpu)
include_hifi_library_headers(graphics)
include_hifi_library_headers(hfm)
