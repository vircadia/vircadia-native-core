cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME "avatar-mixer")

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup the project
include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME})

# link required hifi libraries
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(avatars ${TARGET_NAME} ${ROOT_DIR})