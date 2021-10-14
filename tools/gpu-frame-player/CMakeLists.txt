
set(TARGET_NAME gpu-frame-player)

setup_memory_debugger()
setup_thread_debugger()

if (APPLE)
    set(CMAKE_MACOSX_BUNDLE TRUE)
endif()

setup_hifi_project(Gui Widgets)

if (APPLE)
    unset(CMAKE_MACOSX_BUNDLE)
endif()

# link in the shared libraries
link_hifi_libraries(
    shared ktx shaders gpu
#    vk gpu-vk 
    gl ${PLATFORM_GL_BACKEND}
)

target_compile_definitions(${TARGET_NAME} PRIVATE USE_GL)

set(OpenGL_GL_PREFERENCE "GLVND")
target_opengl()
#target_vulkan()

package_libraries_for_deployment()

if (BUILD_GPU_FRAME_PLAYER_ONLY)

# setup install of executable and things copied by fixup/windeployqt
install(
    DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
    DESTINATION "."
    COMPONENT "client"
    PATTERN "*.pdb" EXCLUDE
    PATTERN "*.lib" EXCLUDE
    PATTERN "*.exp" EXCLUDE
)

endif()
