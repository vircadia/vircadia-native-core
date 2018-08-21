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
  set(BUILD_GLOBAL_SERVICES "DEVELOPMENT")
  set(USE_STABLE_GLOBAL_SERVICES 0)
  set(BUILD_NUMBER 0)
  set(APP_USER_MODEL_ID "com.highfidelity.sandbox-dev")

  set_from_env(RELEASE_TYPE RELEASE_TYPE "DEV")
  set_from_env(RELEASE_NUMBER RELEASE_NUMBER "")
  set_from_env(STABLE_BUILD STABLE_BUILD 0)

  message(STATUS "The RELEASE_TYPE variable is: ${RELEASE_TYPE}")

  # setup component categories for installer
  set(DDE_COMPONENT dde)
  set(CLIENT_COMPONENT client)
  set(SERVER_COMPONENT server)

  if (RELEASE_TYPE STREQUAL "PRODUCTION")
    set(DEPLOY_PACKAGE TRUE)
    set(PRODUCTION_BUILD 1)
    set(BUILD_VERSION ${RELEASE_NUMBER})
    set(BUILD_ORGANIZATION "High Fidelity")
    set(HIGH_FIDELITY_PROTOCOL "hifi")
    set(INTERFACE_BUNDLE_NAME "Interface")
    set(INTERFACE_ICON_PREFIX "interface")

    # add definition for this release type
    add_definitions(-DPRODUCTION_BUILD)

    # if the build is a PRODUCTION_BUILD from the "stable" branch
    # then use the STABLE gobal services
    if (STABLE_BUILD)
      message(STATUS "The RELEASE_TYPE is PRODUCTION and STABLE_BUILD is 1")
      set(BUILD_GLOBAL_SERVICES "STABLE")
      set(USE_STABLE_GLOBAL_SERVICES 1)
    endif ()

    if (NOT BYPASS_SIGNING)
      set(BYPASS_SIGNING 0)
    endif ()      

  elseif (RELEASE_TYPE STREQUAL "PR")
    set(DEPLOY_PACKAGE TRUE)
    set(PR_BUILD 1)
    set(BUILD_VERSION "PR${RELEASE_NUMBER}")
    set(BUILD_ORGANIZATION "High Fidelity - PR${RELEASE_NUMBER}")
    set(INTERFACE_BUNDLE_NAME "Interface")
    set(INTERFACE_ICON_PREFIX "interface-beta")

    # add definition for this release type
    add_definitions(-DPR_BUILD)
  else ()
    set(DEV_BUILD 1)
    set(BUILD_VERSION "dev")
    set(BUILD_ORGANIZATION "High Fidelity - ${BUILD_VERSION}")
    set(INTERFACE_BUNDLE_NAME "Interface")
    set(INTERFACE_ICON_PREFIX "interface-beta")

    # add definition for this release type
    add_definitions(-DDEV_BUILD)
  endif ()

  string(TIMESTAMP BUILD_TIME "%d/%m/%Y")

  # if STABLE_BUILD is 1, PRODUCTION_BUILD must be 1 and
  # DEV_BUILD and PR_BUILD must be 0
  if (STABLE_BUILD)
    if ((NOT PRODUCTION_BUILD) OR PR_BUILD OR DEV_BUILD)
      message(FATAL_ERROR "Cannot produce STABLE_BUILD without PRODUCTION_BUILD")
    endif ()
  endif ()

  if ((PRODUCTION_BUILD OR PR_BUILD) AND NOT STABLE_BUILD)
    # append the abbreviated commit SHA to the build version
    # since this is a PR build or master/nightly builds

    # for PR_BUILDS, we need to grab the abbreviated SHA
    # for the second parent of HEAD (not HEAD) since that is the
    # SHA of the commit merged to master for the build
    if (PR_BUILD)
      set(_GIT_LOG_FORMAT "%p %h")
    else ()
      set(_GIT_LOG_FORMAT "%h")
    endif ()

    execute_process(
      COMMAND git log -1 --abbrev=7 --format=${_GIT_LOG_FORMAT}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE _GIT_LOG_OUTPUT
      ERROR_VARIABLE _GIT_LOG_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (PR_BUILD)
      separate_arguments(_COMMIT_PARENTS UNIX_COMMAND ${_GIT_LOG_OUTPUT})
      list(GET _COMMIT_PARENTS 1 GIT_COMMIT_HASH)
    else ()
      set(GIT_COMMIT_HASH ${_GIT_LOG_OUTPUT})
    endif ()

    if (_GIT_LOG_ERROR OR NOT GIT_COMMIT_HASH)
      message(FATAL_ERROR "Could not retreive abbreviated SHA for PR or production master build")
    endif ()

    set(BUILD_VERSION_NO_SHA ${BUILD_VERSION})
    set(BUILD_VERSION "${BUILD_VERSION}-${GIT_COMMIT_HASH}")

    # pass along a release number without the SHA in case somebody
    # wants to compare master or PR builds as integers
    set(BUILD_NUMBER ${RELEASE_NUMBER})
  endif ()

  if (DEPLOY_PACKAGE)
    # for deployed packages always grab the serverless content
    set(DOWNLOAD_SERVERLESS_CONTENT ON)
  endif ()

  if (APPLE)
    set(DMG_SUBFOLDER_NAME "${BUILD_ORGANIZATION}")

    set(ESCAPED_DMG_SUBFOLDER_NAME "")
    string(REPLACE " " "\\ " ESCAPED_DMG_SUBFOLDER_NAME ${DMG_SUBFOLDER_NAME})

    set(DMG_SUBFOLDER_ICON "${HF_CMAKE_DIR}/installer/install-folder.rsrc")

    set(CONSOLE_INSTALL_DIR ${DMG_SUBFOLDER_NAME})
    set(INTERFACE_INSTALL_DIR ${DMG_SUBFOLDER_NAME})

    set(CONSOLE_EXEC_NAME "Sandbox.app")
    set(CONSOLE_INSTALL_APP_PATH "${CONSOLE_INSTALL_DIR}/${CONSOLE_EXEC_NAME}")

    set(CONSOLE_APP_CONTENTS "${CONSOLE_INSTALL_APP_PATH}/Contents")
    set(COMPONENT_APP_PATH "${CONSOLE_APP_CONTENTS}/MacOS/Components.app")
    set(COMPONENT_INSTALL_DIR "${COMPONENT_APP_PATH}/Contents/MacOS")
    set(CONSOLE_PLUGIN_INSTALL_DIR "${COMPONENT_APP_PATH}/Contents/PlugIns")


    set(INTERFACE_INSTALL_APP_PATH "${CONSOLE_INSTALL_DIR}/${INTERFACE_BUNDLE_NAME}.app")
    set(INTERFACE_ICON_FILENAME "${INTERFACE_ICON_PREFIX}.icns")
  else ()
    if (WIN32)
      set(CONSOLE_INSTALL_DIR "server-console")
    else ()
      set(CONSOLE_INSTALL_DIR ".")
    endif ()

    set(COMPONENT_INSTALL_DIR ".")
    set(INTERFACE_INSTALL_DIR ".")
  endif ()

  if (WIN32)
    set(INTERFACE_EXEC_PREFIX "interface")
    set(INTERFACE_ICON_FILENAME "${INTERFACE_ICON_PREFIX}.ico")

    set(CONSOLE_EXEC_NAME "server-console.exe")

    set(DS_EXEC_NAME "domain-server.exe")
    set(AC_EXEC_NAME "assignment-client.exe")

    # shortcut names
    if (PRODUCTION_BUILD)
      set(INTERFACE_SHORTCUT_NAME "High Fidelity Interface")
      set(CONSOLE_SHORTCUT_NAME "Sandbox")
      set(APP_USER_MODEL_ID "com.highfidelity.sandbox")
    else ()
      set(INTERFACE_SHORTCUT_NAME "High Fidelity Interface - ${BUILD_VERSION_NO_SHA}")
      set(CONSOLE_SHORTCUT_NAME "Sandbox - ${BUILD_VERSION_NO_SHA}")
    endif ()

    set(INTERFACE_HF_SHORTCUT_NAME "${INTERFACE_SHORTCUT_NAME}")
    set(CONSOLE_HF_SHORTCUT_NAME "High Fidelity ${CONSOLE_SHORTCUT_NAME}")

    set(PRE_SANDBOX_INTERFACE_SHORTCUT_NAME "High Fidelity")
    set(PRE_SANDBOX_CONSOLE_SHORTCUT_NAME "Server Console")

    # check if we need to find signtool
    if (PRODUCTION_BUILD OR PR_BUILD)
      if (MSVC_VERSION GREATER_EQUAL 1910) # VS 2017
        find_program(SIGNTOOL_EXECUTABLE signtool PATHS "C:/Program Files (x86)/Windows Kits/10" PATH_SUFFIXES "bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64")
      elseif (MSVC_VERSION GREATER_EQUAL 1800) # VS 2013
        find_program(SIGNTOOL_EXECUTABLE signtool PATHS "C:/Program Files (x86)/Windows Kits/8.1" PATH_SUFFIXES "bin/x64")
      else()
        message( FATAL_ERROR "Visual Studio 2013 or higher required." )
      endif()

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
    set(CLIENT_LAUNCH_NOW_REG_KEY "ClientLaunchAfterInstall")
    set(SERVER_LAUNCH_NOW_REG_KEY "ServerLaunchAfterInstall")
    set(CUSTOM_INSTALL_REG_KEY "CustomInstall")
    set(CLIENT_ID_REG_KEY "ClientGUID")
    set(GA_TRACKING_ID $ENV{GA_TRACKING_ID})
  endif ()

  # print out some results for testing this new build feature
  message(STATUS "The BUILD_GLOBAL_SERVICES variable is: ${BUILD_GLOBAL_SERVICES}")
  message(STATUS "The USE_STABLE_GLOBAL_SERVICES variable is: ${USE_STABLE_GLOBAL_SERVICES}")

  # create a header file our targets can use to find out the application version
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/includes")
  configure_file("${HF_CMAKE_DIR}/templates/BuildInfo.h.in" "${CMAKE_BINARY_DIR}/includes/BuildInfo.h")
  include_directories("${CMAKE_BINARY_DIR}/includes")

endmacro(SET_PACKAGING_PARAMETERS)
