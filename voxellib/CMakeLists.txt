cmake_minimum_required(VERSION 2.8)

project(voxellib)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

# set up the external glm library
include(../cmake/macros/IncludeGLM.cmake)
include_glm(voxellib)

# grab the implemenation and header files
file(GLOB HIFI_VOXELLIB_SRCS src/*.h src/*.cpp)

# create a library and set the property so it can be referenced later
add_library(HifiVoxelLib ${HIFI_VOXELLIB_SRCS})
set(HIFI_VOXELLIB_LIBRARY HifiVoxelLib)

# this is needed on mac? not sure about others
if (NOT WIN32)
    # link target to common, external libraries
    target_link_libraries(HifiVoxelLib)
endif (NOT WIN32)
