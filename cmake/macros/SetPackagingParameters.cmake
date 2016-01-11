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
  if (DEFINED ENV{JOB_ID})
    set(DEPLOY_PACKAGE TRUE)
    set(BUILD_SEQ $ENV{JOB_ID})
    set(PRODUCTION_BUILD TRUE)
    set(INSTALLER_COMPANY "High Fidelity")
    set(INSTALLER_DIRECTORY "${INSTALLER_COMPANY}")
    set(INSTALLER_NAME "interface-win64-${BUILD_SEQ}.exe")
    set(HIGH_FIDELITY_PROTOCOL "hifi")
    set(INTERFACE_BUNDLE_NAME "High Fidelity")
    set(INTERFACE_ICON_PREFIX "interface")
    set(CONSOLE_ICON "console.ico")
  elseif (DEFINED ENV{ghprbPullId})
    set(DEPLOY_PACKAGE TRUE)
    set(PR_BUILD TRUE)
    set(BUILD_SEQ "PR-$ENV{ghprbPullId}")
    set(INSTALLER_COMPANY "High Fidelity - PR")
    set(INSTALLER_DIRECTORY "${INSTALLER_COMPANY}\\${BUILD_SEQ}")
    set(INSTALLER_NAME "pr-interface-win64-${BUILD_SEQ}.exe")
    set(INTERFACE_BUNDLE_NAME "High Fidelity ${BUILD_SEQ}")
    set(INTERFACE_ICON_PREFIX "interface-beta")
    set(CONSOLE_ICON "console-beta.ico")
  else ()
    set(BUILD_SEQ "dev")
    set(DEV_BUILD TRUE)
    set(INSTALLER_COMPANY "High Fidelity - Dev")
    set(INSTALLER_DIRECTORY "${INSTALLER_COMPANY}")
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
  configure_file("${MACRO_DIR}/ApplicationVersion.h.in" "${CMAKE_BINARY_DIR}/includes/ApplicationVersion.h")

endmacro(SET_PACKAGING_PARAMETERS)
