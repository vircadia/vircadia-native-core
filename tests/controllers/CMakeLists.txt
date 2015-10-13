
set(TARGET_NAME controllers-test)
 
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Script Qml)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(shared script-engine plugins input-plugins display-plugins controllers)


if (WIN32)
    add_dependency_external_projects(OpenVR)
    find_package(OpenVR REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()

copy_dlls_beside_windows_executable()