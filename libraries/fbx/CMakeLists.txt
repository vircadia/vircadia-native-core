set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME fbx)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(octree ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(voxels ${TARGET_NAME} "${ROOT_DIR}")

# link ZLIB
find_package(ZLIB)

include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}")
target_link_libraries(${TARGET_NAME} "${ZLIB_LIBRARIES}" Qt5::Widgets)

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()