cmake_minimum_required(VERSION 2.8)

project(shared)

# grab the implemenation and header files
file(GLOB HIFI_SHARED_SRCS src/*.h src/*.cpp)

# create a library and set the property so it can be referenced later
add_library(HifiShared ${HIFI_SHARED_SRCS})
set(HIFI_SHARED_LIBRARY HifiShared)