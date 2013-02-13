cmake_minimum_required(VERSION 2.8)

project(mixer)

file(GLOB MIXER_SRCS src/*.cpp src/*.h)

add_executable(mixer ${MIXER_SRCS} ${HIFI_SHARED_SRCS})

include_directories(${HIFI_SHARED_DIR})

find_package(Threads REQUIRED)
target_link_libraries(interface
    ${CMAKE_THREAD_LIBS_INIT}
)