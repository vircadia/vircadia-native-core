set(PACKAGING_TARGET_NAME package-console)

# add a target that when built will produce an executable of console for this platform
add_custom_target(${PACKAGING_TARGET_NAME}
  COMMAND npm run-script package-darwin
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
