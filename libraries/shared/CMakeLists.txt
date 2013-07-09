cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME shared)
project(${TARGET_NAME})

# grab the implemenation and header files
file(GLOB HIFI_SHARED_SRCS src/*.h src/*.cpp)

# create a library and set the property so it can be referenced later
add_library(${TARGET_NAME} ${HIFI_SHARED_SRCS})
set(HIFI_SHARED_LIBRARY ${TARGET_NAME})

set(EXTERNAL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

if (WIN32)
    # include headers for external libraries and InterfaceConfig.
    include_directories(${EXTERNAL_ROOT_DIR})
else (WIN32)
    find_package(CURL REQUIRED)
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${CURL_LIBRARY})
endif (WIN32)

# link required libraries on UNIX
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif (UNIX AND NOT APPLE)

# include GLM
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})