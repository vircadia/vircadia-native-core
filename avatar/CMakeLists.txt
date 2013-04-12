cmake_minimum_required(VERSION 2.8)

project(avatar)

# grab the implemenation and header files
file(GLOB AVATAR_SRCS src/*.cpp src/*.h)

# add the executable
add_executable(avatar ${AVATAR_SRCS})

# link the shared hifi library
include(../cmake/macros/LinkHifiShared.cmake)
link_hifi_shared_library(avatar)

# link the threads library
find_package(Threads REQUIRED)
target_link_libraries(avatar ${CMAKE_THREAD_LIBS_INIT})
