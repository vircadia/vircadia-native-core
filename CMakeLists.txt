cmake_minimum_required(VERSION 2.8)

project(hifi)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
  set(CMAKE_OSX_SYSROOT macosx)
endif()

add_subdirectory(space)
add_subdirectory(domain)
add_subdirectory(mixer)
add_subdirectory(interface)