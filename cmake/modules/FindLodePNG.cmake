# - Try to find the LodePNG library
#
#  You must provide a LODEPNG_ROOT_DIR which contains the header and cpp file
#
#  Once done this will define
#
#  LODEPNG_FOUND - system has LODEPNG_FOUND
#  LODEPNG_INCLUDE_DIRS - the LodePNG include directory
#  LODEPNG_LIBRARIES- Link these to use LodePNG
#
#  Copyright (c) 2013 Stephen Birarda <birarda@coffeeandpower.com>
#

if (LODEPNG_LIBRARIES AND LODEPNG_INCLUDE_DIRS)
  # in cache already
  set(LODEPNG_FOUND TRUE)
else (LODEPNG_LIBRARIES AND LODEPNG_INCLUDE_DIRS)

  find_path(LODEPNG_INCLUDE_DIRS lodepng.h ${LODEPNG_ROOT_DIR})
  find_file(LODEPNG_LIBRARIES lodepng.cpp ${LODEPNG_ROOT_DIR})

  if (LODEPNG_INCLUDE_DIRS AND LODEPNG_LIBRARIES)
     set(LODEPNG_FOUND TRUE)
  endif (LODEPNG_INCLUDE_DIRS AND LODEPNG_LIBRARIES)
 
  if (LODEPNG_FOUND)
    if (NOT LodePNG_FIND_QUIETLY)
      message(STATUS "Found LodePNG: ${LODEPNG_LIBRARIES}")
    endif (NOT LodePNG_FIND_QUIETLY)
  else (LODEPNG_FOUND)
    if (LodePNG_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LodePNG")
    endif (LodePNG_FIND_REQUIRED)
  endif (LODEPNG_FOUND)

  # show the LODEPNG_INCLUDE_DIRS and LODEPNG_LIBRARIES variables only in the advanced view
  mark_as_advanced(LODEPNG_INCLUDE_DIRS LODEPNG_LIBRARIES)

endif (LODEPNG_LIBRARIES AND LODEPNG_INCLUDE_DIRS)