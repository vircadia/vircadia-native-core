cmake_minimum_required(VERSION 2.8)

project(injector)

# grab the implemenation and header files
file(GLOB INJECTOR_SRCS src/*.cpp src/*.h)

# add the executable
add_executable(injector ${INJECTOR_SRCS})

# link the shared hifi library
include(../cmake/macros/LinkHifiLibrary.cmake)
link_hifi_library(shared injector)

# link the threads library
find_package(Threads REQUIRED)
target_link_libraries(injector ${CMAKE_THREAD_LIBS_INIT})
