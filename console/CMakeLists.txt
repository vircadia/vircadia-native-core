# add a command to produce an executable for this platform
add_custom_target(package-console
  COMMAND npm run-script package-darwin
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
