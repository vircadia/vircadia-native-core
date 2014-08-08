set(TARGET_NAME metavoxel-tests)

auto_mtc(${TARGET_NAME} "${ROOT_DIR}")

setup_hifi_project(${TARGET_NAME} Network Script Widgets)

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} metavoxels networking shared)

# link any shared dependencies
link_shared_dependencies_to_target(${TARGET_NAME})