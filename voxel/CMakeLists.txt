cmake_minimum_required(VERSION 2.8)

project(voxel)

file(GLOB VOXEL_SRCS src/*.cpp src/*.h)

add_executable(voxel ${VOXEL_SRCS})

# link in the shared library
include(../cmake/macros/LinkHifiLibrary.cmake)
link_hifi_library(shared voxel)

# link in the hifi voxels library
link_hifi_library(voxels voxel)