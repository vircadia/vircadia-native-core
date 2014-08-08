set(TARGET_NAME domain-server)

set(ROOT_DIR ..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME} TRUE)

# remove and then copy the files for the webserver
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E remove_directory
                   $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy_directory
                  "${PROJECT_SOURCE_DIR}/resources/web"
                  $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web)

# link the shared hifi library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(embedded-webserver ${TARGET_NAME} "${ROOT_DIR}")

IF (WIN32)
  target_link_libraries(${TARGET_NAME} Winmm Ws2_32)
ENDIF(WIN32)

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()

find_package(Qt5 COMPONENTS Network Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Network)
