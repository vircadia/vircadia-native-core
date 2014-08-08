set(TARGET_NAME particles)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Gui Network Script)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree fbx networking animation)
include_hifi_library_headers(script-engine)