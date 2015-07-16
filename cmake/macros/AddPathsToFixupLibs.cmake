# 
#  AddPathsToFixupLibs.cmake
#  cmake/macros
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 17, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(add_paths_to_fixup_libs)
  foreach(_PATH ${ARGN})
    set(_TEMP_LIB_PATHS ${FIXUP_LIBS})
    
    list(APPEND _TEMP_LIB_PATHS ${_PATH})
    
    list(REMOVE_DUPLICATES _TEMP_LIB_PATHS)
    
    set(FIXUP_LIBS ${_TEMP_LIB_PATHS} CACHE STRING "Paths for external libraries passed to fixup_bundle" FORCE)
  endforeach()
endmacro()