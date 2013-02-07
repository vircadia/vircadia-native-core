cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

project(hifi)

set(GLM_DIR "${CMAKE_SOURCE_DIR}/thirdparty/glm")
set(PORTAUDIO_DIR "${CMAKE_SOURCE_DIR}/thirdparty/PortAudio")

add_subdirectory(interface)