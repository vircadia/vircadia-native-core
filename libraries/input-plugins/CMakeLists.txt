set(TARGET_NAME input-plugins)
setup_hifi_library()
link_hifi_libraries(shared plugins controllers script-engine)

GroupSources("src/input-plugins")

target_sdl2()
target_sixense()
