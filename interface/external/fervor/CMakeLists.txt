cmake_minimum_required(VERSION 2.8)
project(Fervor)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Widgets REQUIRED)

add_definitions(-DFV_GUI)

file(GLOB FERVOR_SOURCES *.cpp)
file(GLOB FERVOR_HEADERS *.h)

file(GLOB FERVOR_UI *.ui)

qt5_wrap_ui(FERVOR_WRAPPED_UI ${FERVOR_UI})

LIST(GET FERVOR_HEADERS 1 FIRST_HEADER)
GET_FILENAME_COMPONENT(HEADER_PATH ${FIRST_HEADER} PATH)
list(REMOVE_ITEM FERVOR_HEADERS ${HEADER_PATH}/fvversioncomparator.h)

file(GLOB FERVOR_UI *.ui)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")
find_package(Quazip REQUIRED)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QUAZIP_INCLUDE_DIRS}
)

add_library(fervor ${FERVOR_SOURCES} ${FERVOR_HEADERS} ${FERVOR_MOC_SOURCES} ${FERVOR_WRAPPED_UI})
target_link_libraries(fervor ${QUAZIP_LIBRARIES})

qt5_use_modules(fervor Core Network Widgets)
