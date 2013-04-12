cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME space-server)

set(MACRO_DIR ../cmake/macros)
include(${MACRO_DIR}/SetupHifiProject.cmake)

setup_hifi_project(${TARGET_NAME})

include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME})