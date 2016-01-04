macro(CONSOLIDATE_INSTALLER_COMPONENTS)

  if (DEFINED DEPLOY_PACKAGE AND DEPLOY_PACKAGE AND WIN32)
    # Copy icon files for interface and stack manager
    if (TARGET_NAME STREQUAL "interface" OR TARGET_NAME STREQUAL "package-console")
      if (TARGET_NAME STREQUAL "interface")
        set (ICON_FILE_PATH "${PROJECT_SOURCE_DIR}/icon/${INTERFACE_ICON}")
        set (ICON_DESTINATION_NAME "interface.ico")
      else ()
        set (ICON_FILE_PATH "${PROJECT_SOURCE_DIR}/assets/${STACK_MANAGER_ICON}")
        set (ICON_DESTINATION_NAME "stack-manager.ico")
      endif ()
      add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy ${ICON_FILE_PATH} ${CMAKE_BINARY_DIR}/package-bundle/${ICON_DESTINATION_NAME}
        COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/package-bundle
      )
    else ()
      add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/package-bundle
      )
    endif ()
  endif ()

endmacro()
