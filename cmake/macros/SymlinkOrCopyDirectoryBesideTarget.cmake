#
#  CopyDirectoryBesideTarget.cmake
#  cmake/macros
#
#  Created by Stephen Birarda on 04/30/15.
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(SYMLINK_OR_COPY_DIRECTORY_BESIDE_TARGET _SHOULD_SYMLINK _DIRECTORY _DESTINATION)

  # remove the current directory
  add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E remove_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/${_DESTINATION}
  )

  if (${_SHOULD_SYMLINK})

    # first create the destination
    add_custom_command(
      TARGET ${TARGET_NAME} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory
      $<TARGET_FILE_DIR:${TARGET_NAME}>/${_DESTINATION}
    )

    # the caller wants a symlink, so just add a command to symlink all contents of _DIRECTORY
    # in the destination - we can't simply create a symlink for _DESTINATION
    # because the remove_directory call above would delete the original files

    file(GLOB _DIR_ITEMS ${_DIRECTORY}/*)

    foreach(_ITEM ${_DIR_ITEMS})
      # get the filename for this item
      get_filename_component(_ITEM_FILENAME ${_ITEM} NAME)

      # add the command to symlink this item
      add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E create_symlink
        ${_ITEM}
        $<TARGET_FILE_DIR:${TARGET_NAME}>/${_DESTINATION}/${_ITEM_FILENAME}
      )
    endforeach()
  else ()
    # copy the directory
    add_custom_command(
      TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_DIRECTORY}
        $<TARGET_FILE_DIR:${TARGET_NAME}>/${_DESTINATION}
    )
  endif ()
  # glob everything in this directory - add a custom command to copy any files
endmacro(SYMLINK_OR_COPY_DIRECTORY_BESIDE_TARGET _SHOULD_SYMLINK _DIRECTORY)
