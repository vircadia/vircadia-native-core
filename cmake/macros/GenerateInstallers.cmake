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

  set(CPACK_MODULE_PATH ${CPACK_MODULE_PATH} "${HF_CMAKE_DIR}/templates")

  set(_DISPLAY_NAME "High Fidelity")

  set(CPACK_PACKAGE_NAME _DISPLAY_NAME)
  set(CPACK_PACKAGE_VENDOR _DISPLAY_NAME)
  set(CPACK_NSIS_DISPLAY_NAME _DISPLAY_NAME)
  set(CPACK_NSIS_PACKAGE_NAME _DISPLAY_NAME)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY _DISPLAY_NAME)

  # configure a cpack properties file for custom installation options
  set(CPACK_CONFIGURED_PROP_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackCustomProperties.cmake")
  configure_file("${HF_CMAKE_DIR}/templates/CPackProperties.cmake.in" ${CPACK_CONFIGURED_PROP_FILE})
  set(CPACK_PROPERTIES_FILE ${CPACK_CONFIGURED_PROP_FILE})

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
endmacro()
