set(TARGET_NAME ui-tests)
 
setup_hifi_project(Widgets OpenGL Network Qml Quick Script)

if (WIN32)
    add_dependency_external_projects(glew)
    find_package(GLEW REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${GLEW_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARIES} wsock32.lib opengl32.lib Winmm.lib)
endif()

# link in the shared libraries
link_hifi_libraries(ui render-utils gpu shared)

copy_dlls_beside_windows_executable()