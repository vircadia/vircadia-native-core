set(TARGET_NAME metavoxel-tests)

set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

find_package(Qt5 COMPONENTS Network Script Widgets)

include(${MACRO_DIR}/AutoMTC.cmake)
auto_mtc(${TARGET_NAME} "${ROOT_DIR}")

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME} TRUE "${AUTOMTC_SRC}")

#include glm
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

# link in the shared libraries
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(metavoxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")

IF (WIN32)
	target_link_libraries(${TARGET_NAME} Winmm Ws2_32)
ENDIF(WIN32)

target_link_libraries(${TARGET_NAME} Qt5::Network Qt5::Widgets Qt5::Script)

