cmake_minimum_required(VERSION 2.8)

project(voxel)

file(GLOB VOXEL_SRCS src/*.cpp src/*.h)

add_executable(voxel ${VOXEL_SRCS})

include(../LinkHifiShared.cmake)
link_hifi_shared_library(voxel)