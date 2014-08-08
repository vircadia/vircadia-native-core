set(TARGET_NAME octree-tests)

setup_hifi_project(${TARGET_NAME} Script Network)

include_glm(${TARGET_NAME})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} animation fbx models networking octree shared)

# link any shared dependencies
link_shared_dependencies_to_target(${TARGET_NAME})