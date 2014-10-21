set(TARGET_NAME octree-tests)

setup_hifi_project(Script Network)

include_glm()

# link in the shared libraries
link_hifi_libraries(shared octree voxels fbx metavoxels networking entities avatars audio animation script-engine)

link_shared_dependencies()
