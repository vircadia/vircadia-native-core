cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME gverb)
project(${TARGET_NAME})

# grab the implementation and header files
file(GLOB GVERB_SRCS src/*.c)

include_directories(include)

add_library(${TARGET_NAME} ${GVERB_SRCS})
