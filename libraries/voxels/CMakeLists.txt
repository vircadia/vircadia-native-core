set(TARGET_NAME voxels)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets Script)

include_glm()

link_hifi_libraries(shared octree networking)

# find ZLIB
find_package(ZLIB REQUIRED)
include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")

# add it to our list of libraries to link
list(APPEND ${TARGET}_LIBRARIES_TO_LINK "${ZLIB_LIBRARIES}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()