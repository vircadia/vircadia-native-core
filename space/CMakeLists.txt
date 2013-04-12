cmake_minimum_required(VERSION 2.8)

project(space)

file(GLOB SPACE_SRCS src/*.cpp src/*.h)

add_executable(space ${SPACE_SRCS})

include(../cmake/macros/LinkHifiShared.cmake)
link_hifi_shared_library(space)