set(TARGET_NAME script-engine)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Gui Network Script Widgets)

include_glm()

link_hifi_libraries(shared octree voxels fbx entities animation audio)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
