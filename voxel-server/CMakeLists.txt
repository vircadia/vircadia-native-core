cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME voxel-server)

set(MACRO_DIR ../cmake/macros)
include(${MACRO_DIR}/SetupHifiProject.cmake)

setup_hifi_project(${TARGET_NAME})

# link in the shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME})

# link in the hifi voxels library
link_hifi_library(voxels ${TARGET_NAME})