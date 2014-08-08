set(TARGET_NAME voxels)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Widgets Script)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree networking)

# link ZLIB
find_package(ZLIB)
include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")

get_target_property(LINKED_LIBRARIES ${TARGET_NAME} DEPENDENCY_LIBRARIES)
list(APPEND DEPENDENCY_LIBRARIES "${ZLIB_LIBRARIES}")
list(REMOVE_DUPLICATES DEPENDENCY_LIBRARIES)