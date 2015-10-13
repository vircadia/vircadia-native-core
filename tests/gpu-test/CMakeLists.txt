set(TARGET_NAME gpu-test)
AUTOSCRIBE_SHADER_LIB(gpu model render-utils)
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL Script Widgets)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
link_hifi_libraries(networking gpu procedural shared fbx model model-networking animation script-engine render-utils )
copy_dlls_beside_windows_executable()