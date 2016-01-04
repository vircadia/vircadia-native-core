set(TARGET_NAME package-console)

# add a target that will package the console
add_custom_target(${TARGET_NAME}
  COMMAND npm run packager -- --out ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "Installer")

consolidate_installer_components()
