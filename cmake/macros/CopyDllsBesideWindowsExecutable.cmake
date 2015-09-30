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
  
  if (WIN32)
    configure_file(
      ${HIFI_CMAKE_DIR}/templates/FixupBundlePostBuild.cmake.in  
      ${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake
      @ONLY
    )
    
    # add a post-build command to copy DLLs beside the executable
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND}
        -DBUNDLE_EXECUTABLE=$<TARGET_FILE:${TARGET_NAME}>
        -P ${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake
    )
    
    find_program(WINDEPLOYQT_COMMAND windeployqt PATHS ${QT_DIR}/bin NO_DEFAULT_PATH)
    
    if (NOT WINDEPLOYQT_COMMAND)
      message(FATAL_ERROR "Could not find windeployqt at ${QT_DIR}/bin. windeployqt is required.")
    endif ()
  
    # add a post-build command to call windeployqt to copy Qt plugins
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND CMD /C "SET PATH=%PATH%;${QT_DIR}/bin && ${WINDEPLOYQT_COMMAND} $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release> $<TARGET_FILE:${TARGET_NAME}>"
    )
  endif ()
endmacro()