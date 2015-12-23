macro(CONSOLIDATE_STACK_COMPONENTS)

  if (DEFINED DEPLOY_PACKAGE AND DEPLOY_PACKAGE)
    if (WIN32)
      # Copy icon files for interface and stack manager
      if (TARGET_NAME STREQUAL "interface" OR TARGET_NAME STREQUAL "stack-manager")
        if (TARGET_NAME STREQUAL "interface")
          set (ICON_FILE_PATH "${PROJECT_SOURCE_DIR}/icon/${INTERFACE_ICON}")
          set (ICON_DESTINATION_NAME "interface.ico")
          set (DEPLOYMENT_PATH "front-end-deployment")
        else ()
          set (ICON_FILE_PATH "${PROJECT_SOURCE_DIR}/assets/${STACK_MANAGER_ICON}")
          set (ICON_DESTINATION_NAME "stack-manager.ico")
          set (DEPLOYMENT_PATH "back-end-deployment")
        endif ()
        add_custom_command(
          TARGET ${TARGET_NAME} POST_BUILD
          COMMAND "${CMAKE_COMMAND}" -E copy ${ICON_FILE_PATH} ${CMAKE_BINARY_DIR}/${DEPLOYMENT_PATH}/${ICON_DESTINATION_NAME}
          COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/${DEPLOYMENT_PATH}
        )
      else ()
        add_custom_command(
          TARGET ${TARGET_NAME} POST_BUILD
          COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/back-end-deployment
        )
      endif ()
    endif ()
  endif ()

endmacro()