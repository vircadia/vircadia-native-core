cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME voxel-server-library)

find_package(Qt5Widgets REQUIRED)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

qt5_use_modules(${TARGET_NAME} Widgets)

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

# setup a library for civetweb and link it to the voxel-server-library
# this assumes that the domain-server cmake has already correctly set up the civetweb library
include_directories(../../domain-server/external/civetweb/include)
target_link_libraries(${TARGET_NAME} civetweb)

include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})

# link ZLIB
find_package(ZLIB)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})

# link in the shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})

# link in the hifi voxels library
link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})

# link in the hifi avatars library
link_hifi_library(avatars ${TARGET_NAME} ${ROOT_DIR})
