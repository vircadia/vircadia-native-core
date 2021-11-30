cmake_minimum_required(VERSION 3.0)
if (APPLE)
  set(ENV{MACOSX_DEPLOYMENT_TARGET} 10.10)
endif()
project(HQLauncher)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
include("cmake/macros/SetPackagingParameters.cmake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include("cmake/init.cmake")
include("cmake/macros/SetPackagingParameters.cmake")

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()


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
  add_compile_options(-W -Wall -Wextra -Wpedantic)
endif()
if (WIN32)

    ExternalProject_Add(
        qtlite
        URL "${EXTERNAL_BUILD_ASSETS}/dependencies/qtlite/qt-lite-5.9.9-win-oct-15-2019.zip"
        URL_HASH MD5=0176277bca37d219e83738caf3a698eb
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
  set(OPENSSL_USE_STATIC_LIBS TRUE)
  find_package(OpenSSL 1.1.0 REQUIRED)

  message("-- Found OpenSSL Libs ${OPENSSL_LIBRARIES}")

endif ()

if (APPLE)
  ExternalProject_Add(
    qtlite
    URL "${EXTERNAL_BUILD_ASSETS}/dependencies/qtlite/qt-lite-5.9.9-mac.zip"
    URL_HASH MD5=0cd78d40e5f539a7e314cf99b6cae0d0
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
  find_package(OpenSSL 1.1.0 REQUIRED)
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

QT5_ADD_RESOURCES(RES_SOURCES ${RESOURCES_QRC})

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
  src/LoginRequest.h
  src/LoginRequest.cpp
  src/SignupRequest.h
  src/SignupRequest.cpp
  src/BuildsRequest.h
  src/BuildsRequest.cpp
  src/UserSettingsRequest.h
  src/UserSettingsRequest.cpp
  src/PathUtils.h
  src/PathUtils.cpp
  src/Unzipper.h
  src/Unzipper.cpp
  src/Helper.h
  src/Helper.cpp
  src/CommandlineOptions.h
  src/CommandlineOptions.cpp
  deps/miniz/miniz.h
  deps/miniz/miniz.cpp
  )


if (APPLE)
  set(src_files ${src_files}
    src/Helper_darwin.mm
    src/NSTask+NSTaskExecveAdditions.h
    src/NSTask+NSTaskExecveAdditions.m
    )
endif()

if (WIN32)
  set(src_files ${src_files}
    src/Helper_windows.cpp
    src/LauncherInstaller_windows.h
    src/LauncherInstaller_windows.cpp
    )
endif()
set(TARGET_NAME ${PROJECT_NAME})


set_packaging_parameters()
if (WIN32)
  set(CONFIGURE_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/interface.ico")
  message(${CONFIGURE_ICON_PATH})
  set(CONFIGURE_ICON_RC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Icon.rc")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/Icon.rc.in" ${CONFIGURE_ICON_RC_OUTPUT})
  add_executable(${PROJECT_NAME} WIN32 ${src_files} ${RES_SOURCES} ${CONFIGURE_ICON_RC_OUTPUT})
elseif (APPLE)
  set(APP_NAME "HQ Launcher")
  set_target_properties(${this_target} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in)

  set(MACOSX_BUNDLE_ICON_FILE "interface.icns")
  add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${src_files} ${RES_SOURCES})
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${APP_NAME}
    MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})


  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${APP_NAME}.app/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources/images "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${APP_NAME}.app/Contents/Resources/"
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${APP_NAME}.app/Contents/MacOS/" ln -sf ./HQ\ Launcher updater
    # Older versions of Launcher put updater in `/Contents/Resources/updater`.
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${APP_NAME}.app/Contents/Resources" ln -sf ../MacOS/HQ\ Launcher updater
    )
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
    #"${SSL_DIR}/lib/libeay32.lib"
    #"${SSL_DIR}/lib/ssleay32.lib"
    ${OPENSSL_LIBRARIES}
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

  if (LAUNCHER_SOURCE_TREE_RESOURCES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_PREFIX_URL="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HIFI_USE_LOCAL_FILE)
    message("Use source tree resources path: file://${CMAKE_CURRENT_SOURCE_DIR}/resources/")
  else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_PREFIX_URL="qrc:/")
    message("Use resource.rcc path: qrc:/")
  endif()

  target_compile_definitions(${PROJECT_NAME} PRIVATE LAUNCHER_BUILD_VERSION="${BUILD_VERSION}")

if (APPLE)
  install(
    TARGETS HQLauncher
    BUNDLE DESTINATION "."
    COMPONENT applications)

  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

  include(CPackComponent)

  set(CPACK_PACKAGE_NAME "HQ Launcher")
  set(CPACK_PACKAGE_VENDOR "Vircadia")
  set(CPACK_PACKAGE_FILE_NAME "HQ Launcher")

  set(CPACK_NSIS_DISPLAY_NAME ${_DISPLAY_NAME})

  set(DMG_SUBFOLDER_NAME "Vircadia")
  set(ESCAPED_DMG_SUBFOLDER_NAME "")
  set(DMG_SUBFOLDER_ICON "${CMAKE_SOURCE_DIR}/cmake/installer/install-folder.rsrc")

  set(CPACK_GENERATOR "DragNDrop")
  include(CPack)
endif()
