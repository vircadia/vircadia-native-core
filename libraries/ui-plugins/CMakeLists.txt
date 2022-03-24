set(TARGET_NAME ui-plugins)
setup_hifi_library(Gui)
link_hifi_libraries(shared shared-gui plugins ui)
include_hifi_library_headers(gpu)
