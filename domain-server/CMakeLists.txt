cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME domain-server)

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

# set up the external glm library
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

include(${MACRO_DIR}/SetupHifiProject.cmake)

# grab cJSON and civetweb sources to pass as OPTIONAL_SRCS
FILE(GLOB OPTIONAL_SRCS ${ROOT_DIR}/externals/civetweb/src/*)

setup_hifi_project(${TARGET_NAME} TRUE ${OPTIONAL_SRCS})

include_directories(SYSTEM ${ROOT_DIR}/externals/civetweb/include)

# remove and then copy the files for the webserver
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E remove_directory
                   $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy_directory
                  "${PROJECT_SOURCE_DIR}/resources/web"
                  $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web)

# link the shared hifi library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})

# link dl library on UNIX for civetweb
if (UNIX AND NOT APPLE)
  target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif (UNIX AND NOT APPLE)