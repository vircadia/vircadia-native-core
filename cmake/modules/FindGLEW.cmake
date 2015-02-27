#
#  FindGLEW.cmake
# 
#  Try to find GLEW library and include path.
#  Once done this will define
#
#  GLEW_FOUND
#  GLEW_INCLUDE_DIRS
#  GLEW_LIBRARIES
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

if (WIN32)
  include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
  hifi_library_search_hints("glew")
  
  find_path(GLEW_INCLUDE_DIRS GL/glew.h PATH_SUFFIXES include HINTS ${GLEW_SEARCH_DIRS})
  
  find_library(GLEW_LIBRARY_RELEASE glew32 PATH_SUFFIXES "lib/Release/Win32" "lib" HINTS ${GLEW_SEARCH_DIRS})
  find_library(GLEW_LIBRARY_DEBUG glew32d PATH_SUFFIXES "lib/Debug/Win32" "lib" HINTS ${GLEW_SEARCH_DIRS})
  
  find_path(GLEW_DLL_PATH glew32.dll PATH_SUFFIXES "bin/Release/Win32" HINTS ${GLEW_SEARCH_DIRS})
  
  include(SelectLibraryConfigurations)
  select_library_configurations(GLEW)
  
  set(GLEW_LIBRARIES ${GLEW_LIBRARY})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_INCLUDE_DIRS GLEW_LIBRARIES GLEW_DLL_PATH)
  
  add_paths_to_fixup_libs(${GLEW_DLL_PATH})
endif ()

