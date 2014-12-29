set(TARGET_NAME fbx)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()

link_hifi_libraries(shared gpu model networking octree voxels)

find_package(ZLIB REQUIRED)
include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")
target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()
