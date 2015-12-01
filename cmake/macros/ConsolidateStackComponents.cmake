macro(CONSOLIDATE_STACK_COMPONENTS)

  if (DEFINED ENV{ghprbPullId} AND WIN32)
  # Copy all the output for this target into the common deployment location
  add_custom_command(
  	TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory $<TARGET_FILE_DIR:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/full-stack-deployment
  )
  endif ()

endmacro()