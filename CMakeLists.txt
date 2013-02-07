cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

project(hifi)

set(GLM_ROOT_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

add_subdirectory(interface)