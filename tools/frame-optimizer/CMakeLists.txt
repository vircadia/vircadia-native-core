set(TARGET_NAME frame-optimizer)
 
setup_memory_debugger()
setup_hifi_project(Gui Widgets)
link_hifi_libraries(shared ktx shaders gpu )
package_libraries_for_deployment()
