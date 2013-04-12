# - Try to find the LodePNG library
#
#  You must provide a LODEPNG_ROOT_DIR which contains the header and cpp file
#
#  Once done this will define
#
#  LODEPNG_FOUND - system has LODEPNG_FOUND
#  LODEPNG_INCLUDE_DIRS - the LodePNG include directory
#  LODEPNG_LIBRARY - Link these to use LodePNG
#
#  Copyright (c) 2013 Stephen Birarda <birarda@coffeeandpower.com>
#

if (LODEPNG_LIBRARY AND LODEPNG_INCLUDE_DIRS)
  # in cache already
  set(LODEPNG_FOUND TRUE)
else (LODEPNG_LIBRARY AND LODEPNG_INCLUDE_DIRS)

  FIND_PATH(LODEPNG_INCLUDE_DIR "lodepng.h"
    PATHS ${LODEPNG_ROOT_DIR})

  set(LODEPNG_INCLUDE_DIRS
    ${LODEPNG_INCLUDE_DIR}
  )

  set(LODEPNG_LIBRARY
    ${LODEPNG_ROOT_DIR}/lodepng.cpp
  )

  if (LODEPNG_INCLUDE_DIRS AND LODEPNG_LIBRARY)
     set(LODEPNG_FOUND TRUE)
  endif (LODEPNG_INCLUDE_DIRS AND LODEPNG_LIBRARY)
 
  if (LODEPNG_FOUND)
    if (NOT LodePNG_FIND_QUIETLY)
      message(STATUS "Found LodePNG: ${LODEPNG_LIBRARY}")
    endif (NOT LodePNG_FIND_QUIETLY)
  else (LODEPNG_FOUND)
    if (LodePNG_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LodePNG")
    endif (LodePNG_FIND_REQUIRED)
  endif (LODEPNG_FOUND)

  # show the LODEPNG_INCLUDE_DIRS and LODEPNG_LIBRARY variables only in the advanced view
  mark_as_advanced(LODEPNG_INCLUDE_DIRS LODEPNG_LIBRARY)

endif (LODEPNG_LIBRARY AND LODEPNG_INCLUDE_DIRS)