cmake_minimum_required(VERSION 3.11)

project(HQLauncher)

set(CMAKE_CXX_STANDARD_REQUIRED_ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


include("cmake/init.cmake")

include(ExternalProject)

if (WIN32)

    ExternalProject_Add(
        qtlite
        URL "https://hifi-public.s3.amazonaws.com/huffman/launcher/qt-lite-ssl.zip"
        URL_HASH MD5=83eeba1565e5727aef11655acf893c15
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
    )


  ExternalProject_Get_Property(qtlite SOURCE_DIR)
  ExternalProject_Get_Property(qtlite STAMP_DIR)

  include("${STAMP_DIR}/download-qtlite.cmake")
  include("${STAMP_DIR}/extract-qtlite.cmake")
  include("${STAMP_DIR}/verify-qtlite.cmake")

  message("${SOURCE_DIR}/lib/cmake")

  list(APPEND CMAKE_PREFIX_PATH ${SOURCE_DIR}/lib/cmake)

  set(SSL_DIR ${SOURCE_DIR}/ssl)
  message("SSL dir is ${SSL_DIR}")

endif ()

find_package(Qt5 COMPONENTS Core Gui Qml Quick QuickControls2 Network REQUIRED)
find_package(OpenGL)

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
  src/Launcher.h
  src/Launcher.cpp
  src/LauncherState.h
  src/LauncherState.cpp
  src/LauncherWindow.h
  src/LauncherWindow.cpp
  src/Unzipper.h
  src/Unzipper.cpp
  deps/miniz/miniz.h
  deps/miniz/miniz.cpp
  )

set(TARGET_NAME ${PROJECT_NAME})
add_executable(${PROJECT_NAME} ${src_files})

#target_openssl()

target_link_libraries(${PROJECT_NAME}
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
  Crypt32 Iphlpapi
  "${SSL_DIR}/lib/libeay32.lib"
  "${SSL_DIR}/lib/ssleay32.lib"
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
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/
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
