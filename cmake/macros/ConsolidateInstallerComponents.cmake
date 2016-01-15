macro(CONSOLIDATE_INSTALLER_COMPONENTS)

  if (DEPLOY_PACKAGE AND WIN32)
    # Copy icon files for interface and stack manager
    if (TARGET_NAME STREQUAL "interface")
      set (ICON_FILE_PATH "${PROJECT_SOURCE_DIR}/icon/${INTERFACE_ICON}")
      set (ICON_DESTINATION_NAME "interface.ico")

      add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy ${ICON_FILE_PATH} ${CMAKE_BINARY_DIR}/package-bundle/${ICON_DESTINATION_NAME}
        COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/package-bundle
      )
     elseif (TARGET_NAME STREQUAL "package-console")
      set (ICON_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/${CONSOLE_ICON}")
      set (ICON_DESTINATION_NAME "console.ico")

      # add a command to copy the folder produced by electron-packager to package-bundle
      add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy ${ICON_FILE_PATH} ${CMAKE_BINARY_DIR}/package-bundle/${ICON_DESTINATION_NAME}
        COMMAND "${CMAKE_COMMAND}" -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER} ${CMAKE_BINARY_DIR}/package-bundle
      )
    else ()
      # add a command to copy the fixed up binary and libraries to package-bundle
      add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/package-bundle
      )
    endif ()
  endif ()

endmacro()
