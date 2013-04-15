cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

set(TARGET_NAME avatars)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})