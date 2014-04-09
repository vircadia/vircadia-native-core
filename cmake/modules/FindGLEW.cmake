#
#  FindGLEW.cmake
# 
#  Try to find GLEW library and include path.
#  Once done this will define
#
#  GLEW_FOUND
#  GLEW_INCLUDE_DIRS
#  GLEW_LIBRARY
# 
#  Created on 2/6/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Adapted from FindGLEW.cmake available in the nvidia-texture-tools repository
#  (https://code.google.com/p/nvidia-texture-tools/source/browse/trunk/cmake/FindGLEW.cmake?r=96)
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (GLEW_INCLUDE_DIRS AND GLEW_LIBRARY)
  set(GLEW_FOUND TRUE)
else ()
  if (WIN32)
    set(WIN_GLEW_SEARCH_DIRS "${GLEW_ROOT_DIR}" "$ENV{GLEW_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/glew")
       
  	find_path(GLEW_INCLUDE_DIRS GL/glew.h PATH_SUFFIXES include HINTS ${WIN_GLEW_SEARCH_DIRS})
    
  	find_library(GLEW_LIBRARY glew32s PATH_SUFFIXES "lib/Release/Win32" "lib" HINTS ${WIN_GLEW_SEARCH_DIRS})
  endif ()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_INCLUDE_DIRS GLEW_LIBRARY)
  
  if (GLEW_FOUND)
    if (NOT GLEW_FIND_QUIETLY)
      message(STATUS "Found GLEW: ${GLEW_LIBRARY}")
    endif ()
  else ()
    if (GLEW_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find GLEW")
    endif ()
  endif ()
endif ()