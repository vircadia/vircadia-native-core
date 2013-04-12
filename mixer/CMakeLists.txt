cmake_minimum_required(VERSION 2.8)

project(mixer)

# grab the implemenation and header files
file(GLOB MIXER_SRCS src/*.cpp src/*.h)

# add the mixer executable
add_executable(mixer ${MIXER_SRCS})

# link the shared hifi library
include(../cmake/macros/LinkHifiShared.cmake)
link_hifi_shared_library(mixer)

# link the threads library
find_package(Threads REQUIRED)
target_link_libraries(mixer ${CMAKE_THREAD_LIBS_INIT})