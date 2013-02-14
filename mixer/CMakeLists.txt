cmake_minimum_required(VERSION 2.8)

project(mixer)

file(GLOB MIXER_SRCS src/*.cpp src/*.h)

add_executable(mixer ${MIXER_SRCS})

include(../LinkHifiShared.cmake)
link_hifi_shared_library(mixer)

find_package(Threads REQUIRED)
target_link_libraries(mixer
    ${CMAKE_THREAD_LIBS_INIT}
    ${HIFI_SHARED_LIBRARY}
)