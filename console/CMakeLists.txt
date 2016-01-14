set(TARGET_NAME packaged-console)

if (PRODUCTION_BUILD)
  set(PRODUCTION_OPTION "--production")
endif()

# add a target that will package the console
add_custom_target(${TARGET_NAME}-npm-install
  COMMAND npm install
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_target(${TARGET_NAME}
  COMMAND npm run packager -- --out ${CMAKE_CURRENT_BINARY_DIR} ${PRODUCTION_OPTION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${TARGET_NAME}-npm-install
)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Console")
set_target_properties(${TARGET_NAME}-npm-install PROPERTIES FOLDER "Console")

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

# install the packaged Server Console in our install directory
if (APPLE)
  install(
    PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER}"
    DESTINATION ${CONSOLE_INSTALL_DIR}
    COMPONENT ${SERVER_COMPONENT}
  )
elseif (WIN32)
  set(CONSOLE_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER}")

  install(
    DIRECTORY "${CONSOLE_DESTINATION}/"
    DESTINATION ${CONSOLE_INSTALL_DIR}
    COMPONENT ${SERVER_COMPONENT}
  )

  # sign the copied server console executable after install
  set(EXECUTABLE_PATH "${CONSOLE_DESTINATION}/${CONSOLE_EXEC_NAME}")
  optional_win_executable_signing()
endif()

if (NOT PR_BUILD AND NOT PRODUCTION_BUILD)
  set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endif ()
