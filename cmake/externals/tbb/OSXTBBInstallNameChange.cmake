# 
#  OSXTBBInstallNameChange.cmake
#  cmake/externals/tbb
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 20, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

# first find the so files in the source dir
set(_TBB_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
file(GLOB_RECURSE _TBB_LIBRARIES "${_TBB_LIBRARY_DIR}/*.dylib")

# raise an error if we found none
if (NOT _TBB_LIBRARIES)
  message(FATAL_ERROR "Did not find any TBB libraries")
endif ()

# find the install_name_tool command
find_program(INSTALL_NAME_TOOL_COMMAND NAMES install_name_tool DOC "Path to the install_name_tool command")

# find the lipo command
find_program(LIPO_COMMAND NAMES lipo DOC "Path to the lipo command")

# enumerate the libraries
foreach(_TBB_LIBRARY ${_TBB_LIBRARIES})
  get_filename_component(_TBB_LIBRARY_FILENAME ${_TBB_LIBRARY} NAME)
  
  set(_INSTALL_NAME_ARGS ${INSTALL_NAME_TOOL_COMMAND} -id ${_TBB_LIBRARY} ${_TBB_LIBRARY_FILENAME})
  
  message(STATUS "${INSTALL_NAME_COMMAND} ${_INSTALL_NAME_ARGS}")
  
  execute_process(
    COMMAND ${INSTALL_NAME_COMMAND} ${_INSTALL_NAME_ARGS}
    WORKING_DIRECTORY ${_TBB_LIBRARY_DIR}
    ERROR_VARIABLE _INSTALL_NAME_ERROR
  )
  
  if (_INSTALL_NAME_ERROR)
    message(FATAL_ERROR "There was an error changing install name for ${_TBB_LIBRARY_FILENAME} - ${_INSTALL_NAME_ERROR}")
  endif ()
endforeach()

