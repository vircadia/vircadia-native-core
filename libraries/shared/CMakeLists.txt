cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

set(TARGET_NAME shared)
project(${TARGET_NAME})

find_package(Qt5Core REQUIRED)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

qt5_use_modules(${TARGET_NAME} Core)

set(EXTERNAL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

if (WIN32)
    # include headers for external libraries and InterfaceConfig.
    include_directories(${EXTERNAL_ROOT_DIR})
else (WIN32)
    find_package(CURL REQUIRED)
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${CURL_LIBRARY})
endif (WIN32)

# link required libraries on UNIX
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif (UNIX AND NOT APPLE)

