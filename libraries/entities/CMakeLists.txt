set(TARGET_NAME entities)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network Script)

include_glm()
include_bullet()

link_hifi_libraries(shared octree fbx networking animation physics)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
