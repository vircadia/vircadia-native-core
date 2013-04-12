cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME "avatar-mixer")

# setup the project
include(../cmake/macros/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME})

# link the shared hifi library
include(../cmake/macros/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME})

# link the threads library
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
