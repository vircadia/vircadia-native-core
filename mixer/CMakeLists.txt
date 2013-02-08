cmake_minimum_required(VERSION 2.8)

project(mixer)

file(GLOB MIXER_SRCS src/*.cpp src/*.h)

add_executable(mixer ${MIXER_SRCS})