cmake_minimum_required(VERSION 2.8)

if (WIN32)
  cmake_policy (SET CMP0020 NEW)
endif (WIN32)

set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME embedded-webserver)

find_package(Qt5Network REQUIRED)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} Qt5::Network)