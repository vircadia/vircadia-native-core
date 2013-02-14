cmake_minimum_required(VERSION 2.8)

project(shared)

file(GLOB HIFI_SHARED_SRCS src/*.h src/*.cpp)

add_library(HifiShared ${HIFI_SHARED_SRCS})
set(HIFI_SHARED_LIBRARY HifiShared)