#  Try to find the PrioVR library
#
#  You must provide a PRIOVR_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  PRIOVR_FOUND - system found PrioVR
#  PRIOVR_INCLUDE_DIRS - the PrioVR include directory
#  PRIOVR_LIBRARIES - Link this to use PrioVR
#
#  Created on 5/12/2014 by Andrzej Kapolka
#  Copyright (c) 2014 High Fidelity
#

find_path(PRIOVR_INCLUDE_DIRS yei_skeletal_api.h ${PRIOVR_ROOT_DIR}/include)

if (WIN32)
  find_library(PRIOVR_LIBRARIES Skeletal_API.lib ${PRIOVR_ROOT_DIR}/lib)
endif (WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PRIOVR DEFAULT_MSG PRIOVR_INCLUDE_DIRS PRIOVR_LIBRARIES)

mark_as_advanced(PRIOVR_INCLUDE_DIRS PRIOVR_LIBRARIES)