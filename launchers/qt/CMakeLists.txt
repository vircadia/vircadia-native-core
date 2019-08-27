cmake_minimum_required(VERSION 3.11)
project(HQLauncher)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)
find_package(Qt5 COMPONENTS Core Gui Qml Quick QuickControls2 Network REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

foreach(plugin ${Qt5Gui_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  message("Plugin ${plugin} is at location ${_loc}")
  set(plugin_libs ${plugin_libs} ${_loc})
endforeach()

foreach(qt_lib_name ${Qt_LIBRARIES})
  message("lib ${qt_lib_name} is at location")
endforeach()

set(src_files
  src/main.cpp)

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
