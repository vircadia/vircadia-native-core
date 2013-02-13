cmake_minimum_required(VERSION 2.8)

project(space)

file(GLOB DOMAIN_SRCS src/*.cpp src/*.h)

add_executable(space ${DOMAIN_SRCS})
