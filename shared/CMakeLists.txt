cmake_minimum_required(VERSION 2.8)

project(shared)

# grab the implemenation and header files
file(GLOB HIFI_SHARED_SRCS src/*.h src/*.cpp)

# create a library and set the property so it can be referenced later
add_library(HifiShared ${HIFI_SHARED_SRCS})
set(HIFI_SHARED_LIBRARY HifiShared)

set(EXTERNAL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

if (WIN32)
# include headers for external libraries and InterfaceConfig.
include_directories(
    ${EXTERNAL_ROOT_DIR}
)
endif (WIN32)
if (NOT WIN32)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
# link target to common, external libraries
target_link_libraries(HifiShared ${CURL_LIBRARY})
endif (NOT WIN32)

# link required libraries on UNIX
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    
    target_link_libraries(HifiShared ${CMAKE_THREAD_LIBS_INIT})
endif (UNIX AND NOT APPLE)