
set(TARGET_NAME render-utils-test)
 
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(render-utils gl gpu gpu-gl shared)

package_libraries_for_deployment()
