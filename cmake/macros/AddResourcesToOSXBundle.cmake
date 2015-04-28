#
#  AddResourcesToOSXBundle.cmake 
#  cmake/macros
#
#  Created by Stephen Birarda on 04/27/15.
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

macro(_RECURSIVELY_SET_PACKAGE_LOCATION _PATH)
  # enumerate the items
  foreach(_ITEM ${ARGN})
    
    if (IS_DIRECTORY "${_ITEM}")
      # recurse into the directory and check for more resources
      file(GLOB _DIR_CONTENTS "${_ITEM}/*") 
      
      # figure out the path for this directory and then call ourselves to recurse into it 
      get_filename_component(_ITEM_PATH ${_ITEM} NAME)
      _recursively_set_package_location("${_PATH}/${_ITEM_PATH}" ${_DIR_CONTENTS}) 
    else ()
      # add any files in the root to the resources folder directly
      SET_SOURCE_FILES_PROPERTIES(${_ITEM} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources${_PATH}")
      list(APPEND DISCOVERED_RESOURCES ${_ITEM})
    endif ()

  endforeach()
endmacro(_RECURSIVELY_SET_PACKAGE_LOCATION _PATH)

macro(ADD_RESOURCES_TO_OS_X_BUNDLE _RSRC_FOLDER)

  # GLOB the resource directory
  file(GLOB _ROOT_ITEMS "${_RSRC_FOLDER}/*")
  
  # recursively enumerate from the root items
  _recursively_set_package_location("" ${_ROOT_ITEMS})

endmacro(ADD_RESOURCES_TO_OS_X_BUNDLE _RSRC_FOLDER)
