set(TARGET_NAME voxels)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets Script)

include_glm()

link_hifi_libraries(shared octree networking)

# find ZLIB
find_package(ZLIB REQUIRED)
include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")

# add it to our list of libraries to link
target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()