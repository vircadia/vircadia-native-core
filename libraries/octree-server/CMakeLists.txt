cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME octree-server)

find_package(Qt5Widgets REQUIRED)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)

# grab cJSON and civetweb sources to pass as OPTIONAL_SRCS 
FILE(GLOB OPTIONAL_SRCS ${ROOT_DIR}/externals/civetweb/src/*)

setup_hifi_library(${TARGET_NAME} ${OPTIONAL_SRCS})

include_directories(${ROOT_DIR}/externals/civetweb/include)

qt5_use_modules(${TARGET_NAME} Widgets)

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

# link ZLIB
find_package(ZLIB)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})

# link in the shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})

# link in the hifi octree library
link_hifi_library(octree ${TARGET_NAME} ${ROOT_DIR})

# link dl library on UNIX for civetweb
if (UNIX AND NOT APPLE)
  target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif (UNIX AND NOT APPLE)