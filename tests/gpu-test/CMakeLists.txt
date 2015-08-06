
set(TARGET_NAME gpu-test)
 
AUTOSCRIBE_SHADER_LIB(gpu model render gpu-test)

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

#include_oglplus()

# link in the shared libraries
link_hifi_libraries(render-utils gpu shared networking fbx model animation)

copy_dlls_beside_windows_executable()