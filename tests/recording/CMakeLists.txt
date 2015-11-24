set(TARGET_NAME recording-test)
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Test)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
link_hifi_libraries(shared recording)
package_libraries_for_deployment()

# FIXME convert to unit tests
# Declare dependencies
#macro (setup_testcase_dependencies)
#  # link in the shared libraries
#  link_hifi_libraries(shared recording)
#
#  package_libraries_for_deployment()
#endmacro ()
#setup_hifi_testcase()
