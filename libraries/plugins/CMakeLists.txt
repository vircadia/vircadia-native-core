set(TARGET_NAME plugins)
setup_hifi_library(Gui)
link_hifi_libraries(shared shared-gui networking)
include_hifi_library_headers(gpu)
