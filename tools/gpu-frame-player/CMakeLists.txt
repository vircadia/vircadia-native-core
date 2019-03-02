
set(TARGET_NAME gpu-frame-player)
 
setup_memory_debugger()
setup_hifi_project(Gui Widgets)


# link in the shared libraries
link_hifi_libraries(
    shared ktx shaders gpu
#    vk gpu-vk 
    gl ${PLATFORM_GL_BACKEND}
)

target_compile_definitions(${TARGET_NAME} PRIVATE USE_GL)
target_opengl()
#target_vulkan()

package_libraries_for_deployment()
