set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME metavoxels)

find_package(Qt5 COMPONENTS Network Script Widgets)

include(${MACRO_DIR}/AutoMTC.cmake)
auto_mtc(${TARGET_NAME} "${ROOT_DIR}")

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME} "${AUTOMTC_SRC}")

# link in the networking library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

target_link_libraries(${TARGET_NAME} Qt5::Network Qt5::Widgets Qt5::Script)

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()