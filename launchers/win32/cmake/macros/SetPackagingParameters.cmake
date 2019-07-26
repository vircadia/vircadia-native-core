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
  set(BUILD_NUMBER 0)

  set_from_env(RELEASE_TYPE RELEASE_TYPE "DEV")
  set_from_env(RELEASE_NUMBER RELEASE_NUMBER "")
  set_from_env(STABLE_BUILD STABLE_BUILD 0)

  message(STATUS "The RELEASE_TYPE variable is: ${RELEASE_TYPE}")
  set(BUILD_NUMBER ${RELEASE_NUMBER})

  if (RELEASE_TYPE STREQUAL "PRODUCTION")
    set(PRODUCTION_BUILD 1)
    set(BUILD_VERSION ${RELEASE_NUMBER})

    # add definition for this release type
    add_definitions(-DPRODUCTION_BUILD)

  elseif (RELEASE_TYPE STREQUAL "PR")
    set(PR_BUILD 1)
    set(BUILD_VERSION "PR${RELEASE_NUMBER}")

    # add definition for this release type
    add_definitions(-DPR_BUILD)
  else ()
    set(DEV_BUILD 1)
    set(BUILD_VERSION "dev")
  endif ()

endmacro(SET_PACKAGING_PARAMETERS)
