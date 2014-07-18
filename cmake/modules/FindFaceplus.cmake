#  Try to find the Faceplus library
#
#  You must provide a FACEPLUS_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  FACEPLUS_FOUND - system found Faceplus
#  FACEPLUS_INCLUDE_DIRS - the Faceplus include directory
#  FACEPLUS_LIBRARIES - Link this to use Faceplus
#
#  Created on 4/8/2014 by Andrzej Kapolka
#  Copyright (c) 2014 High Fidelity
#

find_path(FACEPLUS_INCLUDE_DIRS faceplus.h ${FACEPLUS_ROOT_DIR}/include)

if (WIN32)
  find_library(FACEPLUS_LIBRARIES faceplus.lib ${FACEPLUS_ROOT_DIR}/win32/)
endif (WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FACEPLUS DEFAULT_MSG FACEPLUS_INCLUDE_DIRS FACEPLUS_LIBRARIES)

mark_as_advanced(FACEPLUS_INCLUDE_DIRS FACEPLUS_LIBRARIES)