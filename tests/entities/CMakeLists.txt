
set(TARGET_NAME "entities-test")

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Network Script)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(entities avatars shared octree gpu model fbx networking animation audio gl)

package_libraries_for_deployment()
