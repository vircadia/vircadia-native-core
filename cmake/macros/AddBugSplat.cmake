#
#  AddBugSplat.cmake
#  cmake/macros
#
#  Created by Ryan Huffman on 02/09/16.
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

macro(add_bugsplat)
  get_property(BUGSPLAT_CHECKED GLOBAL PROPERTY CHECKED_FOR_BUGSPLAT_ONCE)

  if (NOT BUGSPLAT_CHECKED)
    find_package(BugSplat)
    set_property(GLOBAL PROPERTY CHECKED_FOR_BUGSPLAT_ONCE TRUE)
  endif ()

  if (BUGSPLAT_FOUND)
    add_definitions(-DHAS_BUGSPLAT)

    target_include_directories(${TARGET_NAME} PRIVATE ${BUGSPLAT_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${BUGSPLAT_LIBRARIES})
    add_paths_to_fixup_libs(${BUGSPLAT_DLL_PATH})

    add_custom_command(TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${BUGSPLAT_RC_DLL_PATH} "$<TARGET_FILE_DIR:${TARGET_NAME}>/")
    add_custom_command(TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${BUGSPLAT_EXE_PATH} "$<TARGET_FILE_DIR:${TARGET_NAME}>/")
  endif ()
endmacro()
