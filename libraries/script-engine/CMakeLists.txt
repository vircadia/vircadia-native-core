set(TARGET_NAME script-engine)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Gui Network Script Widgets)

include_glm()

link_hifi_libraries(shared octree voxels gpu model fbx entities animation audio physics metavoxels)

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()

