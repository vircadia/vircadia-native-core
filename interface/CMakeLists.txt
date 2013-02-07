cmake_minimum_required(VERSION 2.8)

project(interface)

file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)

add_executable(interface ${INTERFACE_SRCS})

include_directories(${GLM_DIR})
target_link_libraries(interface GLM)

include_directories(${PORTAUDIO_DIR})
target_link_libraries(interface ${PORTAUDIO_DIR}/libportaudio.a)