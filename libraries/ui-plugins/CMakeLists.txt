set(TARGET_NAME ui-plugins)
setup_hifi_library(Gui)
link_hifi_libraries(shared plugins ui)
include_hifi_library_headers(gpu)