set(TARGET_NAME input-plugins)
setup_hifi_library()
link_hifi_libraries(shared plugins ui-plugins controllers ui)
include_hifi_library_headers(script-engine)

GroupSources("src/input-plugins")
