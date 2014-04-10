cmake_minimum_required(VERSION 2.8)

if (WIN32)
  cmake_policy (SET CMP0020 NEW)
endif (WIN32)

set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME octree)

find_package(Qt5Widgets REQUIRED)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")

# link ZLIB and GnuTLS
find_package(ZLIB)
find_package(GnuTLS REQUIRED)

include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}" "${GNUTLS_INCLUDE_DIR}")

target_link_libraries(${TARGET_NAME} "${ZLIB_LIBRARIES}" Qt5::Widgets "${GNUTLS_LIBRARY}")