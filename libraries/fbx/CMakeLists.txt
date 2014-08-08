set(TARGET_NAME fbx)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm("${ROOT_DIR}")

link_hifi_libraries(shared networking octree voxels)

find_package(ZLIB REQUIRED)
include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")
list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${ZLIB_LIBRARIES}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
