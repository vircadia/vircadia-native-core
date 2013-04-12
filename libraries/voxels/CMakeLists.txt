cmake_minimum_required(VERSION 2.8)

set(MACRO_DIR ../../cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME voxels)

project(${TARGET_NAME})

# set up the external glm library
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${MACRO_DIR})

# grab the implemenation and header files
file(GLOB HIFI_VOXELS_SRCS src/*.h src/*.cpp)

# create a library and set the property so it can be referenced later
add_library(${TARGET_NAME} ${HIFI_VOXELS_SRCS})
set(HIFI_VOXELS_LIBRARY ${TARGET_NAME})