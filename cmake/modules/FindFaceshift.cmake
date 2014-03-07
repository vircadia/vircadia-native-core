#  Try to find the Faceshift networking library
#
#  You must provide a FACESHIFT_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  FACESHIFT_FOUND - system found Faceshift
#  FACESHIFT_INCLUDE_DIRS - the Faceshift include directory
#  FACESHIFT_LIBRARIES - Link this to use Faceshift
#
#  Created on 8/30/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (FACESHIFT_LIBRARIES AND FACESHIFT_INCLUDE_DIRS)
  # in cache already
  set(FACESHIFT_FOUND TRUE)
else ()
  find_path(FACESHIFT_INCLUDE_DIRS fsbinarystream.h ${FACESHIFT_ROOT_DIR}/include)

  if (APPLE)
    find_library(FACESHIFT_LIBRARIES libfaceshift.a ${FACESHIFT_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(FACESHIFT_LIBRARIES libfaceshift.a ${FACESHIFT_ROOT_DIR}/lib/UNIX/)
  elseif (WIN32)
	# For windows, we're going to build the faceshift sources directly into the interface build
	# and not link to a prebuilt library. This is because the VS2010 linker doesn't like cross-linking
	# between release and debug libraries. If we change that in the future we can make win32 more
	# like the other platforms
    #find_library(FACESHIFT_LIBRARIES faceshift.lib ${FACESHIFT_ROOT_DIR}/lib/WIN32/)
  endif ()

  if (WIN32)
    # Windows only cares about the headers
    if (FACESHIFT_INCLUDE_DIRS)
      set(FACESHIFT_FOUND TRUE)
    endif (FACESHIFT_INCLUDE_DIRS AND FACESHIFT_LIBRARIES)
  else ()
    # Mac and Unix requires libraries
    if (FACESHIFT_INCLUDE_DIRS AND FACESHIFT_LIBRARIES)
      set(FACESHIFT_FOUND TRUE)
    endif (FACESHIFT_INCLUDE_DIRS AND FACESHIFT_LIBRARIES)
  endif ()

  if (FACESHIFT_FOUND)
    if (NOT FACESHIFT_FIND_QUIETLY)
      message(STATUS "Found Faceshift... ${FACESHIFT_LIBRARIES}")
    endif (NOT FACESHIFT_FIND_QUIETLY)
  else ()
    if (FACESHIFT_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Faceshift")
    endif (FACESHIFT_FIND_REQUIRED)
  endif ()

  # show the FACESHIFT_INCLUDE_DIRS and FACESHIFT_LIBRARIES variables only in the advanced view
  mark_as_advanced(FACESHIFT_INCLUDE_DIRS FACESHIFT_LIBRARIES)

endif ()
