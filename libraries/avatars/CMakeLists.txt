set(TARGET_NAME avatars)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Network Script)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree voxels networking)
include_hifi_library_headers(fbx)