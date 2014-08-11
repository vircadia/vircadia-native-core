set(TARGET_NAME voxel-edit)

setup_hifi_project()

include_glm()

link_hifi_libraries(networking octree shared voxels)

link_shared_dependencies()