set(TARGET_NAME input-plugins)
setup_hifi_library()
link_hifi_libraries(shared plugins ui-plugins controllers ui)

GroupSources("src/input-plugins")
