#
#  SetPackagingParameters.cmake
#  cmake/macros
#
#  Created by Leonardo Murillo on 07/14/2015.
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

# This macro checks some Jenkins defined environment variables to determine the origin of this build
# and decides how targets should be packaged.

macro(SET_PACKAGING_PARAMETERS)
  set(PR_BUILD 0)
  set(PRODUCTION_BUILD 0)
  set(DEV_BUILD 0)

  if (DEFINED ENV{JOB_ID})
    set(DEPLOY_PACKAGE TRUE)
    set(BUILD_SEQ $ENV{JOB_ID})
    set(PRODUCTION_BUILD 1)
    set(BUILD_ORGANIZATION "High Fidelity")
    set(INSTALLER_NAME "interface-win64-${BUILD_SEQ}.exe")
    set(HIGH_FIDELITY_PROTOCOL "hifi")
    set(INTERFACE_BUNDLE_NAME "High Fidelity")
    set(INTERFACE_ICON_PREFIX "interface")
    set(CONSOLE_ICON "console.ico")
  elseif (DEFINED ENV{ghprbPullId})
    set(DEPLOY_PACKAGE TRUE)
    set(PR_BUILD 1)
    set(BUILD_SEQ "PR-$ENV{ghprbPullId}")
    set(BUILD_ORGANIZATION "High Fidelity - PR#$ENV{ghprbPullId}")
    set(INSTALLER_NAME "pr-interface-win64-${BUILD_SEQ}.exe")
    set(INTERFACE_BUNDLE_NAME "High Fidelity ${BUILD_SEQ}")
    set(INTERFACE_ICON_PREFIX "interface-beta")
    set(CONSOLE_ICON "console-beta.ico")
  else ()
    set(BUILD_SEQ "dev")
    set(DEV_BUILD 1)
    set(BUILD_ORGANIZATION "High Fidelity - Dev")
    set(INSTALLER_NAME "dev-interface-win64.exe")
    set(INTERFACE_BUNDLE_NAME "Interface")
    set(INTERFACE_ICON_PREFIX "interface-beta")
    set(CONSOLE_ICON "console-beta.ico")
  endif ()

  set(CONSOLE_INSTALL_DIR ".")
  set(INTERFACE_INSTALL_DIR ".")

  if (WIN32)
    set(INTERFACE_EXEC_PREFIX "interface")
    set(INTERFACE_ICON_FILENAME "${INTERFACE_ICON_PREFIX}.ico")

    set(CONSOLE_EXEC_NAME "server-console.exe")

    # start menu shortcuts
    set(INTERFACE_SM_SHORTCUT_NAME "High Fidelity")
    set(CONSOLE_SM_SHORTCUT_NAME "Server Console")

    # check if we need to find signtool
    if (PRODUCTION_BUILD OR PR_BUILD)
      find_program(SIGNTOOL_EXECUTABLE signtool PATHS "C:/Program Files (x86)/Windows Kits/8.1" PATH_SUFFIXES "bin/x64")

      if (NOT SIGNTOOL_EXECUTABLE)
        message(FATAL_ERROR "Code signing of executables was requested but signtool.exe could not be found.")
      endif ()
    endif ()

    set(GENERATED_UNINSTALLER_EXEC_NAME "Uninstall.exe")
    set(REGISTRY_HKLM_INSTALL_ROOT "Software")
    set(POST_INSTALL_OPTIONS_REG_GROUP "PostInstallOptions")
    set(CLIENT_DESKTOP_SHORTCUT_REG_KEY "ClientDesktopShortcut")
    set(CONSOLE_DESKTOP_SHORTCUT_REG_KEY "ConsoleDesktopShortcut")
    set(CONSOLE_STARTUP_REG_KEY "ConsoleStartupShortcut")
    set(LAUNCH_NOW_REG_KEY "LaunchAfterInstall")
  endif ()

  if (APPLE)
    set(CONSOLE_EXEC_NAME "Server Console.app")
    set(CONSOLE_INSTALL_APP_PATH "${CONSOLE_EXEC_NAME}")

    set(INTERFACE_INSTALL_APP_PATH "${INTERFACE_BUNDLE_NAME}.app")

    set(INTERFACE_ICON_FILENAME "${INTERFACE_ICON_PREFIX}.icns")
  endif()

  # setup component categories for installer
  set(DDE_COMPONENT dde)
  set(CLIENT_COMPONENT client)
  set(SERVER_COMPONENT server)

  # create a header file our targets can use to find out the application version
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/includes")
  configure_file("${HF_CMAKE_DIR}/templates/BuildInfo.h.in" "${CMAKE_BINARY_DIR}/includes/BuildInfo.h")

endmacro(SET_PACKAGING_PARAMETERS)
