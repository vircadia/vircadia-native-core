cmake_minimum_required(VERSION 3.11)
project(HQLauncher)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)
include("cmake/init.cmake")
find_package(Qt5 COMPONENTS Core Gui Qml Quick QuickControls2 Network REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CUSTOM_LAUNCHER_QRC_PATHS "")
set(RESOURCES_QRC ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc)
set(RESOURCES_RCC ${CMAKE_CURRENT_SOURCE_DIR}/resources.rcc)
generate_qrc(OUTPUT ${RESOURCES_QRC} PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources CUSTOM_PATHS ${CUSTOM_LAUNCHER_QRC_PATHS} GLOBS *)

add_custom_command(
  OUTPUT ${RESOURCES_RCC}
  DEPENDS ${RESOURCES_QRC} ${GENERATE_QRC_DEPENDS}
  COMMAND "${_qt5Core_install_prefix}/bin/rcc"
  ARGS ${RESOURCES_QRC} -binary -o ${RESOURCES_RCC})

list(APPEND GENERATE_QRC_DEPENDS ${RESOURCES_RCC})
add_custom_target(resources ALL DEPENDS ${GENERATE_QRC_DEPENDS})

foreach(plugin ${Qt5Gui_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  message("Plugin ${plugin} is at location ${_loc}")
  set(plugin_libs ${plugin_libs} ${_loc})
endforeach()

set(src_files
  src/main.cpp
  src/LauncherWindow.h
  src/LauncherWindow.cpp)

add_executable(${PROJECT_NAME} ${src_files})

target_link_libraries(${PROJECT_NAME} PUBLIC
  Qt5::Core
  Qt5::Quick
  Qt5::QuickControls2
  Qt5::Qml
  Qt5::Gui
  Qt5::Network
  ${Qt_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${plugin_libs}
  wsock32 ws2_32 Winmm version imm32 dwmapi
  "${_qt5Core_install_prefix}/lib/qtpcre2.lib"
  "${_qt5Core_install_prefix}/lib/qtlibpng.lib"
  "${_qt5Core_install_prefix}/lib/qtfreetype.lib"
  "${_qt5Core_install_prefix}/lib/Qt5AccessibilitySupport.lib"
  "${_qt5Core_install_prefix}/lib/Qt5FbSupport.lib"
  "${_qt5Core_install_prefix}/lib/Qt5OpenGLExtensions.lib"
  "${_qt5Core_install_prefix}/lib/Qt5QuickTemplates2.lib"
  "${_qt5Core_install_prefix}/lib/Qt5FontDatabaseSupport.lib"
  "${_qt5Core_install_prefix}/lib/Qt5ThemeSupport.lib"
#  "${_qt5Core_install_prefix}/lib/Qt5Bootstrap.lib"
  "${_qt5Core_install_prefix}/lib/Qt5EventDispatcherSupport.lib"
  "${_qt5Core_install_prefix}/qml/QtQuick.2/qtquick2plugin.lib"
  "${_qt5Core_install_prefix}/qml/QtQuick/Controls.2/qtquickcontrols2plugin.lib"
  "${_qt5Core_install_prefix}/qml/QtQuick/Templates.2/qtquicktemplates2plugin.lib")

target_include_directories(${PROJECT_NAME} PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Quick_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Qml_INCLUDE_DIRS})


set(INTERFACE_EXEC_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
set(RESOURCES_DEV_DIR "${INTERFACE_EXEC_DIR}/resources")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${RESOURCES_RCC}"
  "${INTERFACE_EXEC_DIR}")
