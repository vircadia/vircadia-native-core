cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME faceshift)
project(${TARGET_NAME})

# grab the implemenation and header files
file(GLOB FACESHIFT_SRCS include/*.h src/*.cpp)

include_directories(include)

add_library(${TARGET_NAME} "${FACESHIFT_SRCS}")
