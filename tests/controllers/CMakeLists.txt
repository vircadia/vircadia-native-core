
set(TARGET_NAME controllers-test)
 
AUTOSCRIBE_SHADER_LIB(gpu model render-utils )

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Script)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(shared script-engine input-plugins controllers)

copy_dlls_beside_windows_executable()