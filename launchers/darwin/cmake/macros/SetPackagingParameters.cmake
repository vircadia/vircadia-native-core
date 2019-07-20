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
  set(APP_USER_MODEL_ID "com.highfidelity.console-dev")

  set_from_env(RELEASE_TYPE RELEASE_TYPE "DEV")
  set_from_env(RELEASE_NUMBER RELEASE_NUMBER "")
  set_from_env(STABLE_BUILD STABLE_BUILD 0)

  message(STATUS "The RELEASE_TYPE variable is: ${RELEASE_TYPE}")
  set(BUILD_NUMBER ${RELEASE_NUMBER})

  if (RELEASE_TYPE STREQUAL "PRODUCTION")
    set(DEPLOY_PACKAGE TRUE)
    set(PRODUCTION_BUILD 1)
    set(BUILD_VERSION ${RELEASE_NUMBER})
    set(BUILD_ORGANIZATION "High Fidelity")

    # add definition for this release type
    add_definitions(-DPRODUCTION_BUILD)

  elseif (RELEASE_TYPE STREQUAL "PR")
    set(DEPLOY_PACKAGE TRUE)
    set(PR_BUILD 1)
    set(BUILD_VERSION "PR${RELEASE_NUMBER}")
    set(BUILD_ORGANIZATION "High Fidelity - PR${RELEASE_NUMBER}")

    # add definition for this release type
    add_definitions(-DPR_BUILD)
  else ()
    set(DEV_BUILD 1)
    set(BUILD_VERSION "dev")
    set(BUILD_ORGANIZATION "High Fidelity - ${BUILD_VERSION}")

    # add definition for this release type
    add_definitions(-DDEV_BUILD)
  endif ()

 

  # create a header file our targets can use to find out the application version
  #file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/includes")
  #configure_file("${HF_CMAKE_DIR}/templates/BuildInfo.h.in" "${CMAKE_BINARY_DIR}/includes/BuildInfo.h")
  #include_directories("${CMAKE_BINARY_DIR}/includes")

endmacro(SET_PACKAGING_PARAMETERS)
