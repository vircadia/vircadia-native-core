set(TARGET_NAME fbx-test)
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui)
setup_memory_debugger()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

file(GLOB_RECURSE GLB_TEST_FILES "c:/Users/bdavi/git/glTF-Sample-Models/2.0/*.glb")
list(JOIN GLB_TEST_FILES "|" GLB_TEST_FILES)
target_compile_definitions(${TARGET_NAME} PRIVATE -DGLB_TEST_FILES="${GLB_TEST_FILES}")
link_hifi_libraries(shared graphics networking image gpu hfm fbx)
package_libraries_for_deployment()
