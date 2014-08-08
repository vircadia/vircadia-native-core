set(TARGET_NAME script-engine)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Gui Network Script Widgets)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree voxels fbx particles models animation)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies_to_target(${TARGET_NAME})