set(TARGET_NAME octree-tests)

setup_hifi_project(Script Network)

include_glm()

# link in the shared libraries
link_hifi_libraries(animation fbx entities networking octree shared)

link_shared_dependencies()
