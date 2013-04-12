cmake_minimum_required(VERSION 2.8)
set(MACRO_DIR ../cmake/macros)

set(TARGET_NAME domain-server)

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME})

# link the shared hifi library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME})