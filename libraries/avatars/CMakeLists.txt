cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME avatars)

find_package(Qt5 REQUIRED QtCore)
include(${QT_USE_FILE})
qt5_wrap_cpp(WRAPPED_SRCS src/AvatarData.h)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})

# link in the hifi voxels library
link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})
