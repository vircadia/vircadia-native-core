set(TARGET_NAME packaged-server-console)

if (CLIENT_ONLY AND APPLE)
  # Don't include Console.app in client-only OSX DMGs.
  return()
endif()

if (PRODUCTION_BUILD)
  set(PRODUCTION_OPTION "--production")
endif()

if (CLIENT_ONLY)
  set(CLIENT_ONLY_OPTION "--client_only")
endif()

# add a target that will package the console
add_custom_target(${TARGET_NAME}-npm-install
  COMMAND npm install
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_target(${TARGET_NAME}
  COMMAND npm run packager -- --out ${CMAKE_CURRENT_BINARY_DIR} ${PRODUCTION_OPTION} ${CLIENT_ONLY_OPTION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${TARGET_NAME}-npm-install
)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Server Console")
set_target_properties(${TARGET_NAME}-npm-install PROPERTIES FOLDER "hidden/Server Console")

# add a dependency from the package target to the server components
if (BUILD_CLIENT)
  if (APPLE)
    add_dependencies(${TARGET_NAME} Vircadia)
  else()
    add_dependencies(${TARGET_NAME} interface)
  endif()
endif()

if (BUILD_SERVER)
  add_dependencies(${TARGET_NAME} assignment-client domain-server)
endif()

# set the packaged console folder depending on platform, so we can copy it
if (APPLE)
  if (CLIENT_ONLY)
    set(PACKAGED_CONSOLE_FOLDER "Console-darwin-x64/${CONSOLE_EXEC_NAME}")
  else ()
    set(PACKAGED_CONSOLE_FOLDER "Sandbox-darwin-x64/${CONSOLE_EXEC_NAME}")
  endif()
elseif (WIN32)
  set(PACKAGED_CONSOLE_FOLDER "server-console-win32-x64")
elseif (UNIX)
  set(PACKAGED_CONSOLE_FOLDER "server-console-linux-x64")
endif ()

# install the packaged Server Console in our install directory
if (APPLE)
  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER}"
    USE_SOURCE_PERMISSIONS
    DESTINATION ${CONSOLE_INSTALL_DIR}
  )
elseif (WIN32)
  set(CONSOLE_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER}")

  install(
    DIRECTORY "${CONSOLE_DESTINATION}/"
    DESTINATION ${CONSOLE_INSTALL_DIR}
  )

  # sign the copied server console executable after install
  set(EXECUTABLE_PATH "${CONSOLE_DESTINATION}/${CONSOLE_EXEC_NAME}")
  optional_win_executable_signing()
endif()

if (PR_BUILD OR PRODUCTION_BUILD)
  set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL FALSE EXCLUDE_FROM_DEFAULT_BUILD FALSE)

  if (APPLE)
    set(BESIDE_CONSOLE_DIR "${CONSOLE_INSTALL_APP_PATH}/Contents/Resources")
  else ()
    set(BESIDE_CONSOLE_DIR ${CONSOLE_INSTALL_DIR})
  endif ()

  # configure our build info json file and install it beside the console
  set(CONSOLE_BUILD_INFO_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build-info.json")
  configure_file("${HF_CMAKE_DIR}/templates/console-build-info.json.in" ${CONSOLE_BUILD_INFO_OUTPUT})
  install(
    FILES ${CONSOLE_BUILD_INFO_OUTPUT}
    DESTINATION ${BESIDE_CONSOLE_DIR}
    COMPONENT ${SERVER_COMPONENT}
  )
else ()
  set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  set_target_properties(${TARGET_NAME}-npm-install PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endif ()
