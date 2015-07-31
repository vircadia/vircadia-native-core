
set(TARGET_NAME render-utils-test)
 
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

#include_oglplus()

# link in the shared libraries
link_hifi_libraries(render-utils gpu shared)


include_directories("${PROJECT_BINARY_DIR}/../../libraries/render-utils/")
message(${PROJECT_BINARY_DIR})
copy_dlls_beside_windows_executable()
