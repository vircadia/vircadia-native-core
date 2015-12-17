set(PACKAGING_TARGET_NAME package-console)

# add a target that when built will produce an executable of console for this platform
if (APPLE)
  set(PACKAGE_COMMAND package-darwin)
elseif (WIN32)
  set(PACKAGE_COMMAND package-win)
elseif (UNIX)
  set(PACKAGE_COMMAND package-linux)
endif ()

add_custom_target(${PACKAGING_TARGET_NAME}
  COMMAND npm run-script ${PACKAGE_COMMAND}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
