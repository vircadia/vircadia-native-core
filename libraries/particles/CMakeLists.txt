set(TARGET_NAME particles)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Gui Network Script)

include_glm()

link_hifi_libraries(shared octree fbx networking animation)
include_hifi_library_headers(script-engine)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()