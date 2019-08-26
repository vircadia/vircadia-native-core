cmake_minimum_required(VERSION 3.11)
project(HQLauncher)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)
find_package(Qt5 COMPONENTS Core Gui Qml Quick QuickControls2 Network REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

foreach(plugin ${Qt5Gui_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  message("Plugin ${plugin} is at location ${_loc}")
  set(plugin_libs ${plugin_libs} ${_loc})
endforeach()

set(src_files
  src/main.cpp)

add_executable(${PROJECT_NAME} ${src_file})

target_link_libraries(${PROJECT_NAME} PUBLIC
  Qt5::Core
  Qt5::Quick
  Qt5::QuickControls2
  Qt5::Qml
  Qt5::Gui
  ${OPENGL_LIBRARIES}
  ${plugin_libs})

target_include_directories(${PROJECT_NAME} PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Quick_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Qml_INCLUDE_DIRS})
