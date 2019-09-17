cmake_minimum_required(VERSION 3.10)
project(HQLauncher)

if (APPLE)
  set(ENV{MACOSX_DEPLOYMENT_TARGET} 10.9)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
include("cmake/macros/SetPackagingParameters.cmake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include("cmake/init.cmake")
include("cmake/macros/SetPackagingParameters.cmake")


function(set_from_env _RESULT_NAME _ENV_VAR_NAME _DEFAULT_VALUE)
  if (NOT DEFINED ${_RESULT_NAME})
    if ("$ENV{${_ENV_VAR_NAME}}" STREQUAL "")
      set (${_RESULT_NAME} ${_DEFAULT_VALUE} PARENT_SCOPE)
    else()
      set (${_RESULT_NAME} $ENV{${_ENV_VAR_NAME}} PARENT_SCOPE)
    endif()
  endif()
endfunction()

include(ExternalProject)

if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "-framework Cocoa -framework CoreServices -framework Carbon -framework IOKit -framework Security -framework SystemConfiguration")
endif()
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

if (APPLE)
  ExternalProject_Add(
    qtlite
    URL "https://dantescalves.com/launcher/qt-lite-clang-8.1.0.zip"
    URL_HASH MD5=f8462bfcd54e6b05ef888f8b197149c2
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
  set(OPENSSL_ROOT_DIR ${SSL_DIR})
  message("SSL dir is ${SSL_DIR}")
endif()

if (APPLE)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
  find_package(OpenSSL REQUIRED)
endif()

find_package(Qt5 COMPONENTS Core Gui Qml Quick QuickControls2 Network REQUIRED)
find_package(OpenGL REQUIRED)
find_package(QtStaticDeps REQUIRED)

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
  src/PathUtils.cpp
  src/PathUtils.h
  src/Unzipper.h
  src/Unzipper.cpp
  src/Helper.h
  src/Helper.cpp
  deps/miniz/miniz.h
  deps/miniz/miniz.cpp
  )


if (APPLE)
  set(src_files ${src_files}
    src/darwin/Helper.mm
    src/darwin/NSTask+NSTaskExecveAdditions.h
    src/darwin/NSTask+NSTaskExecveAdditions.m
    )
endif()
set(TARGET_NAME ${PROJECT_NAME})


set_packaging_parameters()
if (WIN32)
  add_executable(${PROJECT_NAME} ${src_files})
elseif (APPLE)
  set_target_properties(${this_target} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in)

  set(MACOSX_BUNDLE_ICON_FILE "interface.icns")
  add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${src_files})
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

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
  ${QT_STATIC_LIBS}
  )

if (WIN32)
  target_link_libraries(${PROJECT_NAME}
    wsock32 ws2_32 Winmm version imm32 dwmapi
    Crypt32 Iphlpapi
    "${SSL_DIR}/lib/libeay32.lib"
    "${SSL_DIR}/lib/ssleay32.lib"
    "${_qt5Core_install_prefix}/qml/QtQuick.2/qtquick2plugin.lib"
    "${_qt5Core_install_prefix}/qml/QtQuick/Controls.2/qtquickcontrols2plugin.lib"
    "${_qt5Core_install_prefix}/qml/QtQuick/Templates.2/qtquicktemplates2plugin.lib")
elseif (APPLE)
  target_link_libraries(${PROJECT_NAME}
    ${OPENSSL_LIBRARIES}
    "${_qt5Core_install_prefix}/qml/QtQuick.2/libqtquick2plugin.a"
    "${_qt5Core_install_prefix}/qml/QtQuick/Controls.2/libqtquickcontrols2plugin.a"
    "${_qt5Core_install_prefix}/qml/QtQuick/Templates.2/libqtquicktemplates2plugin.a"
    "${_qt5Core_install_prefix}/plugins/platforms/libqcocoa.a")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Quick_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Qml_INCLUDE_DIRS})

add_dependencies(${PROJECT_NAME} resources)

if (APPLE)
  target_include_directories(${PROJECT_NAME} PUBLIC
    ${OPENSSL_INCLUDE_DIR})
endif()

set(INTERFACE_EXEC_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
set(RESOURCES_DEV_DIR "${INTERFACE_EXEC_DIR}/resources")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${RESOURCES_RCC}"
  "${INTERFACE_EXEC_DIR}")


if (LAUNCHER_SOURCE_TREE_RESOURCES)
  target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_PREFIX_URL="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
  message("Use source tree resources path: file://${CMAKE_CURRENT_SOURCE_DIR}/resources/")
else()
  target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_PREFIX_URL="qrc:/")
  message("Use resource.rcc path: qrc:/")
endif()

if (APPLE)
  install(
    TARGETS HQLauncher
    BUNDLE DESTINATION "."
    COMPONENT applications)

  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

  include(CPackComponent)

  set(CPACK_PACKAGE_NAME "HQ Launcher")
  set(CPACK_PACKAGE_VENDOR "High Fidelity")
  set(CPACK_PACKAGE_FILE_NAME "HQ Launcher")

  set(CPACK_NSIS_DISPLAY_NAME ${_DISPLAY_NAME})

  set(DMG_SUBFOLDER_NAME "High Fidelity")
  set(ESCAPED_DMG_SUBFOLDER_NAME "")
  set(DMG_SUBFOLDER_ICON "${CMAKE_SOURCE_DIR}/cmake/installer/install-folder.rsrc")

  set(CPACK_GENERATOR "DragNDrop")
  include(CPack)
endif()
