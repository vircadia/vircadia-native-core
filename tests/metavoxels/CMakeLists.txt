set(TARGET_NAME metavoxel-tests)

auto_mtc()

include_glm()

setup_hifi_project(Network Script Widgets)

# link in the shared libraries
link_hifi_libraries(metavoxels networking shared)

link_shared_dependencies()