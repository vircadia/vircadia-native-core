# 
#  HifiLibrarySearchHints.cmake
# 
#  Created by Stephen Birarda on July 24th, 2014
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(HIFI_LIBRARY_SEARCH_HINTS LIBRARY_FOLDER)
  string(TOUPPER ${LIBRARY_FOLDER} LIBRARY_PREFIX)
  set(${LIBRARY_PREFIX}_SEARCH_DIRS "")
  
  if (${LIBRARY_PREFIX}_ROOT_DIR)
    set(${LIBRARY_PREFIX}_SEARCH_DIRS "${${LIBRARY_PREFIX}_ROOT_DIR}")
  endif ()
  
  if (ANDROID)
    set(${LIBRARY_PREFIX}_SEARCH_DIRS "${${LIBRARY_PREFIX}_SEARCH_DIRS}" "/${LIBRARY_FOLDER}")
  endif ()
  
  if (DEFINED ENV{${LIBRARY_PREFIX}_ROOT_DIR})
    set(${LIBRARY_PREFIX}_SEARCH_DIRS "${${LIBRARY_PREFIX}_SEARCH_DIRS}" "$ENV{${LIBRARY_PREFIX}_ROOT_DIR}")
  endif ()
  
  if (DEFINED ENV{HIFI_LIB_DIR})
    set(${LIBRARY_PREFIX}_SEARCH_DIRS "${${LIBRARY_PREFIX}_SEARCH_DIRS}" "$ENV{HIFI_LIB_DIR}/${LIBRARY_FOLDER}")
  endif ()
  
endmacro(HIFI_LIBRARY_SEARCH_HINTS _library_folder)