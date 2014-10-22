#  FindGVerb.cmake
# 
#  Try to find the Gverb library.
#
#  You must provide a GVERB_ROOT_DIR which contains src and include directories
#
#  Once done this will define
#
#  GVERB_FOUND - system found Gverb
#  GVERB_INCLUDE_DIRS - the Gverb include directory
#
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (GVERB_INCLUDE_DIRS)
  # in cache already
  set(GVERB_FOUND TRUE)
else ()
  find_path(GVERB_INCLUDE_DIRS gverb.h ${GVERB_ROOT_DIR}/include)

  if (GVERB_INCLUDE_DIRS)
    set(GVERB_FOUND TRUE)
  endif (GVERB_INCLUDE_DIRS)

  if (GVERB_FOUND)
    if (NOT GVERB_FIND_QUIETLY)
      message(STATUS "Found Gverb: ${GVERB_INCLUDE_DIRS}")
    endif (NOT GVERB_FIND_QUIETLY)
  else ()
    if (GVERB_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find Gverb")
    endif (GVERB_FIND_REQUIRED)
  endif ()

endif ()
