cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

set(TARGET_NAME pairing-server)

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME} TRUE)

# link the shared hifi library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})