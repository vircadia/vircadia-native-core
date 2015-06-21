set(TARGET_NAME render-utils-tests)
 
setup_hifi_project(Quick Gui OpenGL)

#include_oglplus()

# link in the shared libraries
link_hifi_libraries(render-utils gpu shared)

include_directories("${PROJECT_BINARY_DIR}")
message(${PROJECT_BINARY_DIR})
copy_dlls_beside_windows_executable()
