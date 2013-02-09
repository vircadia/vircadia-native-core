cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

project(hifi)

set(GLM_ROOT_DIR ${CMAKE_SOURCE_DIR}/external)
set(LODEPNG_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/LodePNG)
set(PORTAUDIO_DIR ${CMAKE_SOURCE_DIR}/external/portaudio)

add_subdirectory(interface)