set(TARGET_NAME packaged-console)

if (PRODUCTION_BUILD)
  set(PRODUCTION_OPTION "--production")
endif()

# add a target that will package the console
add_custom_target(${TARGET_NAME} ALL
  COMMAND npm run packager -- --out ${CMAKE_CURRENT_BINARY_DIR} ${PRODUCTION_OPTION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# add a dependency from the package target to the server components
add_dependencies(${TARGET_NAME} assignment-client domain-server)

# set the packaged console folder depending on platform, so we can copy it
if (APPLE)
  set(PACKAGED_CONSOLE_FOLDER "Server\\ Console-darwin-x64/Server Console.app")
elseif (WIN32)
  set(PACKAGED_CONSOLE_FOLDER "server-console-win32-x64")
elseif (UNIX)
  set(PACKAGED_CONSOLE_FOLDER "server-console-linux-x64")
endif ()

# install the packaged Server Console
install(
  DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER}"
  DESTINATION ${CONSOLE_INSTALL_DIR}
  COMPONENT ${SERVER_COMPONENT}
)

if (NOT PR_BUILD AND NOT PRODUCTION_BUILD)
  set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endif ()
