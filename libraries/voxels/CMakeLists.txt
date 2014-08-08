set(TARGET_NAME voxels)

setup_hifi_library(${TARGET_NAME})

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree networking)

# link ZLIB
find_package(ZLIB)
find_package(Qt5 COMPONENTS Widgets Script)

include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")
target_link_libraries(${TARGET_NAME} "${ZLIB_LIBRARIES}" Qt5::Widgets Qt5::Script)

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()