set(TARGET_NAME octree-tests)

setup_hifi_project(${TARGET_NAME})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} octree)

# link any shared dependencies
link_shared_dependencies_to_target(${TARGET_NAME})