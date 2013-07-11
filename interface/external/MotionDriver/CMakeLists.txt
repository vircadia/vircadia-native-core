cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME MotionDriver)
project(${TARGET_NAME})

# let the library know which device we're using
add_definitions(-DMPU9150)

# grab the implemenation and header files
file(GLOB MOTION_DRIVER_SRCS include/*.h src/*.c)

include_directories(include)

add_library(${TARGET_NAME} ${MOTION_DRIVER_SRCS})
