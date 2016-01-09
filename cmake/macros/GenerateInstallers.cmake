#
#  GenerateInstallers.cmake
#  cmake/macros
#
#  Created by Leonardo Murillo on 12/16/2015.
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(GENERATE_INSTALLERS)
  include(CPackComponent)

  set(CPACK_PACKAGE_NAME "HighFidelity")
  set(CPACK_PACKAGE_VENDOR "HighFidelity")

  if (WIN32)
    set(INTERFACE_STARTMENU_NAME "High Fidelity")
  endif ()

  if (APPLE)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "/")
    set(CPACK_PACKAGING_INSTALL_PREFIX /)
    set(CPACK_OSX_PACKAGE_VERSION ${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif ()

  # setup downloads
  cpack_configure_downloads(
    http://hifi-production.s3.amazonaws.com/optionals/
    ADD_REMOVE
  )

  set(CLIENT_GROUP client-group)

  # add a component group for the client
  cpack_add_component_group(
    ${CLIENT_GROUP}
    DISPLAY_NAME "Client"
    EXPANDED
  )

  cpack_add_component(
    ${CLIENT_COMPONENT}
    DISPLAY_NAME "High Fidelity Client"
    GROUP ${CLIENT_GROUP}
  )

  if (WIN32 AND DEFINED ENV{DDE_ARCHIVE_DIR})
    # add a download component for DDE
    cpack_add_component(
      ${DDE_COMPONENT}
      DISPLAY_NAME "Webcam Body Movement"
      DEPENDS ${CLIENT_COMPONENT}
      DOWNLOADED
      DISABLE
      GROUP ${CLIENT_GROUP}
      ARCHIVE_FILE "DDE"
    )
  endif ()

  cpack_add_component(${SERVER_COMPONENT}
    DISPLAY_NAME "High Fidelity Server"
  )

  if (APPLE)
    # we don't want the OS X package to install anywhere but the main volume, so disable relocation
    set(CPACK_PACKAGE_RELOCATABLE FALSE)
  endif ()

  include(CPack)

  # if (DEPLOY_PACKAGE AND WIN32)
  #   file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/package-bundle")
  #   find_program(MAKENSIS_COMMAND makensis PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\NSIS])
  #   if (NOT MAKENSIS_COMMAND)
  #     message(FATAL_ERROR "The Nullsoft Scriptable Install Systems is required for generating packaged installers on Windows (http://nsis.sourceforge.net/)")
  #   endif ()
  #   add_custom_target(
  #     build-package ALL
  #     DEPENDS interface assignment-client domain-server stack-manager
  #     COMMAND set INSTALLER_SOURCE_DIR=${CMAKE_BINARY_DIR}/package-bundle
  #     COMMAND set INSTALLER_NAME=${CMAKE_BINARY_DIR}/${INSTALLER_NAME}
  #     COMMAND set INSTALLER_SCRIPTS_DIR=${CMAKE_SOURCE_DIR}/examples
  #     COMMAND set INSTALLER_COMPANY=${INSTALLER_COMPANY}
  #     COMMAND set INSTALLER_DIRECTORY=${INSTALLER_DIRECTORY}
  #     COMMAND CMD /C "\"${MAKENSIS_COMMAND}\" ${CMAKE_SOURCE_DIR}/tools/nsis/release.nsi"
  #   )
  #
  #   set_target_properties(build-package PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "Installer")
  # endif ()
endmacro()
