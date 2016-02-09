macro(add_bugsplat)
  find_package(BugSplat)

  get_property(BUGSPLAT_CHECKED GLOBAL PROPERTY CHECKED_FOR_BUGSPLAT_ONCE)

  if (BUGSPLAT_FOUND)
    add_definitions(-DHAS_BUGSPLAT)

    target_include_directories(${TARGET_NAME} PRIVATE ${BUGSPLAT_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${BUGSPLAT_LIBRARIES})

    add_custom_command(TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${BUGSPLAT_RC_DLL_PATH} "$<TARGET_FILE_DIR:${TARGET_NAME}>/")
    add_custom_command(TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${BUGSPLAT_EXE_PATH} "$<TARGET_FILE_DIR:${TARGET_NAME}>/")
  endif ()
endmacro()
