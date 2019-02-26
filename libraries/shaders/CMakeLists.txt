set(TARGET_NAME shaders)
autoscribe_shader_libs(gpu graphics display-plugins procedural render render-utils entities-renderer)
setup_hifi_library(Gui)
add_dependencies(${TARGET_NAME} scribed_shaders spirv_shaders reflected_shaders)

link_hifi_libraries(shared gl)
target_json()
