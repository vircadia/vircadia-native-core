set(TARGET_NAME package-console)

if (BUILD_TAGGED_BETA)
  set(BETA_OPTION "--beta")
endif()

# add a target that will package the console
add_custom_target(${TARGET_NAME} ALL
  COMMAND npm run packager -- --out ${CMAKE_CURRENT_BINARY_DIR} ${BETA_OPTION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# set the packaged console folder depending on platform, so we can copy it
if (APPLE)
  set(PACKAGED_CONSOLE_FOLDER "Server\\ Console-darwin-x64")
elseif (WIN32)
  set(PACKAGED_CONSOLE_FOLDER "server-console-win32-x64")
elseif (UNIX)
  set(PACKAGED_CONSOLE_FOLDER "server-console-linux-x64")
endif ()

install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_CONSOLE_FOLDER}/Server Console.app" DESTINATION bin COMPONENT ${APP_COMPONENT})

consolidate_installer_components()
