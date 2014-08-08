set(TARGET_NAME fbx)

setup_hifi_library(${TARGET_NAME})

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared networking octree voxels)

# link ZLIB
find_package(ZLIB)

include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES "${ZLIB_LIBRARIES}")
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES ${DEPENDENCY_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})
