cmake_minimum_required(VERSION 2.8)

project(voxel)

file(GLOB VOXEL_SRCS src/*.cpp src/*.h)

add_executable(voxel ${VOXEL_SRCS})

include(../cmake/macros/LinkHifiShared.cmake)
link_hifi_shared_library(voxel)

# link in the hifi voxellib library
include(../cmake/macros/LinkHifiVoxelLib.cmake)
link_hifi_voxellib_library(voxel)
