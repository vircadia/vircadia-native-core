set(TARGET_NAME display-plugins)
setup_hifi_library(OpenGL)
link_hifi_libraries(shared plugins gl ui)

target_opengl()

GroupSources("src/display-plugins")

target_oglplus()
