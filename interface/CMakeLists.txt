cmake_minimum_required (VERSION 2.8)
project (interface)

file (GLOB INTERFACE_SRCS src/*.cpp src/*.h)

add_executable (interface ${INTERFACE_SRCS})

find_package (GLM REQUIRED)
include_directories (${GLM_INCLUDE_DIRS})
target_link_libraries (interface GLM)