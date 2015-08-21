# 
#  OSXInstallNameChange.cmake
#  cmake/macros
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 20, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

# first find the so files in the source dir
message("INSTALL_NAME_LIBRARY_DIR ${INSTALL_NAME_LIBRARY_DIR}")

file(GLOB_RECURSE _LIBRARIES "${INSTALL_NAME_LIBRARY_DIR}/*.dylib")

if (NOT _LIBRARIES)
  message(FATAL_ERROR "OSXInstallNameChange -- no libraries found: ${INSTALL_NAME_LIBRARY_DIR}")
endif ()

# find the install_name_tool command
find_program(INSTALL_NAME_TOOL_COMMAND NAMES install_name_tool DOC "Path to the install_name_tool command")

# enumerate the libraries
foreach(_LIBRARY ${_LIBRARIES})
  get_filename_component(_LIBRARY_FILENAME ${_LIBRARY} NAME)
  
  set(_INSTALL_NAME_ARGS ${INSTALL_NAME_TOOL_COMMAND} -id ${_LIBRARY} ${_LIBRARY_FILENAME})
  
  message(STATUS "${INSTALL_NAME_COMMAND} ${_INSTALL_NAME_ARGS}")
  
  execute_process(
    COMMAND ${INSTALL_NAME_COMMAND} ${_INSTALL_NAME_ARGS}
    WORKING_DIRECTORY ${INSTALL_NAME_LIBRARY_DIR}
    ERROR_VARIABLE _INSTALL_NAME_ERROR
  )
  
  if (_INSTALL_NAME_ERROR)
    message(FATAL_ERROR "There was an error changing install name for ${_LIBRARY_FILENAME} - ${_INSTALL_NAME_ERROR}")
  endif ()
endforeach()

