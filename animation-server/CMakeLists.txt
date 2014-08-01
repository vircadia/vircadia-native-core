if (WIN32)
  cmake_policy (SET CMP0020 NEW)
endif (WIN32)

set(TARGET_NAME animation-server)

set(ROOT_DIR ..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

find_package(Qt5 COMPONENTS Script)
include_directories(SYSTEM "${Qt5Script_INCLUDE_DIRS}")

# set up the external glm library
include("${MACRO_DIR}/IncludeGLM.cmake")
include_glm(${TARGET_NAME} "${ROOT_DIR}")

include("${MACRO_DIR}/SetupHifiProject.cmake")
setup_hifi_project(${TARGET_NAME} TRUE)

# link in the shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")

# link in the hifi octree library
link_hifi_library(octree ${TARGET_NAME} "${ROOT_DIR}")

# link in the hifi voxels library
link_hifi_library(voxels ${TARGET_NAME} "${ROOT_DIR}")

# link the hifi networking library
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()
