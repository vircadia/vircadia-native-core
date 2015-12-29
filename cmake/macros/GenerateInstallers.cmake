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
  if (DEFINED DEPLOY_PACKAGE AND DEPLOY_PACKAGE AND WIN32)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/full-stack-deployment")
    find_program(MAKENSIS_COMMAND makensis PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\NSIS])
    if (NOT MAKENSIS_COMMAND)
      message(FATAL_ERROR "The Nullsoft Scriptable Install Systems is required for generating packaged installers on Windows (http://nsis.sourceforge.net/)")
    endif ()
    add_custom_target(
      build-package ALL
      DEPENDS interface assignment-client domain-server stack-manager
      COMMAND set INSTALLER_SOURCE_DIR=${CMAKE_BINARY_DIR}/full-stack-deployment
      COMMAND set INSTALLER_NAME=${CMAKE_BINARY_DIR}/${INSTALLER_NAME}
      COMMAND set INSTALLER_SCRIPTS_DIR=${CMAKE_SOURCE_DIR}/examples
      COMMAND set INSTALLER_COMPANY=${INSTALLER_COMPANY}
      COMMAND set INSTALLER_DIRECTORY=${INSTALLER_DIRECTORY}
      COMMAND CMD /C "\"${MAKENSIS_COMMAND}\" ${CMAKE_SOURCE_DIR}/tools/nsis/release.nsi"
    )
  endif ()
endmacro()