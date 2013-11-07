cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME assignment-client)

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

find_package(Qt5Network REQUIRED)

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME} TRUE)

qt5_use_modules(${TARGET_NAME} Network)

# include glm
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

# link in the shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(audio ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(avatars ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(voxel-server-library ${TARGET_NAME} ${ROOT_DIR})

include_directories(${ROOT_DIR}/externals/civetweb/include)

if (UNIX)
  target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif (UNIX)

# link curl for synchronous script downloads
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${CURL_LIBRARY})
