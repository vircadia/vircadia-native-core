set(TARGET_NAME octree-tests)

setup_hifi_project(Script Network)

include_glm()

# link in the shared libraries
link_hifi_libraries(animation avatars fbx entities networking octree shared voxels)

link_shared_dependencies()
