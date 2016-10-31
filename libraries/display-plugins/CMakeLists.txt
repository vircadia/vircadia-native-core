set(TARGET_NAME display-plugins)
AUTOSCRIBE_SHADER_LIB(gpu display-plugins)
setup_hifi_library(OpenGL)
link_hifi_libraries(shared plugins ui-plugins gl gpu-gl ui render-utils)

target_opengl()

GroupSources("src/display-plugins")
