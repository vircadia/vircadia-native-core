# 
#  IncludeApplicationVersion.cmake
#  cmake/macros
#
#  Created by Leonardo Murillo on 07/14/2015.
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(INCLUDE_APPLICATION_VERSION)
  #
  # We are relying on Jenkins defined environment variables to determine the origin of this build
  # and will only package if this is a PR or Release build
  if (DEFINED ENV{JOB_ID})
    set(DEPLOY_PACKAGE 1)
    set(BUILD_SEQ $ENV{JOB_ID})
    set(INSTALLER_COMPANY "High Fidelity")
    set(INSTALLER_DIRECTORY "${INSTALLER_COMPANY}")
    set(INSTALLER_NAME "interface-win64-${BUILD_SEQ}.exe")
  elseif (DEFINED ENV{ghprbPullId})
    set(DEPLOY_PACKAGE 1)
    set(BUILD_SEQ "PR-$ENV{ghprbPullId}")
    set(INSTALLER_COMPANY "High Fidelity - PR")
    set(INSTALLER_DIRECTORY "${INSTALLER_COMPANY}\\${BUILD_SEQ}")
    set(INSTALLER_NAME "pr-interface-win64-${BUILD_SEQ}.exe")
  else ()
    set(BUILD_SEQ "dev")
    set(INSTALLER_COMPANY "High Fidelity - Dev")
    set(INSTALLER_DIRECTORY "${INSTALLER_COMPANY}")
    set(INSTALLER_NAME "dev-interface-win64.exe")
  endif ()
  configure_file("${MACRO_DIR}/ApplicationVersion.h.in" "${PROJECT_BINARY_DIR}/includes/ApplicationVersion.h")
  include_directories("${PROJECT_BINARY_DIR}/includes")
endmacro(INCLUDE_APPLICATION_VERSION)
