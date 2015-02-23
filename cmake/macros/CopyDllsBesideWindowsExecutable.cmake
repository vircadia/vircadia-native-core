# 
#  CopyDllsBesideWindowsExecutable.cmake
#  cmake/macros
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 17, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(COPY_DLLS_BESIDE_WINDOWS_EXECUTABLE)
  if (WIN32 AND NOT UPPER_CMAKE_BUILD_TYPE MATCHES RELEASE)
    
    configure_file(
      ${HIFI_CMAKE_DIR}/templates/FixupBundlePostBuild.cmake.in  
      ${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake
      @ONLY
    )
    
    # add a post-build command to copy DLLs beside the interface executable
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND}
        -DBUNDLE_EXECUTABLE=$<TARGET_FILE:${TARGET_NAME}>
        -P ${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake
    )
  endif ()
endmacro()