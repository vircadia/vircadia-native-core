cmake_minimum_required(VERSION 2.8)

project(shared)

# grab the implemenation and header files
file(GLOB HIFI_SHARED_SRCS src/*.h src/*.cpp)

# create a library and set the property so it can be referenced later
add_library(HifiShared ${HIFI_SHARED_SRCS})
set(HIFI_SHARED_LIBRARY HifiShared)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
# link target to common, external libraries
target_link_libraries(HifiShared ${CURL_LIBRARY})

# link required libraries on UNIX
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    
    target_link_libraries(HifiShared ${CMAKE_THREAD_LIBS_INIT})
endif (UNIX AND NOT APPLE)