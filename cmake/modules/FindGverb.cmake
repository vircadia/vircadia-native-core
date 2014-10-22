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
else (GVERB_INCLUDE_DIRS)

  include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
  hifi_library_search_hints("gverb")

  find_path(GVERB_INCLUDE_DIRS gverb.h PATH_SUFFIXES include HINTS ${GVERB_SEARCH_DIRS})
  find_path(GVERB_SRC_DIRS gverb.c PATH_SUFFIXES src HINTS ${GVERB_SEARCH_DIRS})

  if (GVERB_INCLUDE_DIRS)
    set(GVERB_FOUND TRUE)
  endif (GVERB_INCLUDE_DIRS)

  if (GVERB_FOUND)
    message(STATUS "Found Gverb: ${GVERB_INCLUDE_DIRS}")
  else (GVERB_FOUND)
    message(FATAL_ERROR "Could NOT find Gverb. Read ./interface/externals/gverb/readme.txt")
  endif (GVERB_FOUND)

endif(GVERB_INCLUDE_DIR)