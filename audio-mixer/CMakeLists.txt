cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

set(TARGET_NAME audio-mixer)

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME} TRUE)

# set up the external glm library
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

# link the shared hifi library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
# link_hifi_library(audio ${TARGET_NAME} ${ROOT_DIR})

# link the stk library
set(STK_ROOT_DIR ${ROOT_DIR}/externals/stk)
find_package(STK REQUIRED)
target_link_libraries(${TARGET_NAME} ${STK_LIBRARIES})
include_directories(${STK_INCLUDE_DIRS})