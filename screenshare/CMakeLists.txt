set(TARGET_NAME screenshare)

add_custom_target(${TARGET_NAME}-npm-install
	COMMAND npm install
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_target(${TARGET_NAME}
	COMMAND npm run packager -- --out ${CMAKE_CURRENT_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${TARGET_NAME}-npm-install
)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "screenshare")
set_target_properties(${TARGET_NAME}-npm-install PROPERTIES FOLDER "hidden/screenshare")

if (WIN32)
	set(PACKAGED_SCREENSHARE_FOLDER "hifi-screenshare-win32-x64")
	set(SCREENSHARE_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGED_SCREENSHARE_FOLDER}")
	install(
		DIRECTORY "${SCREENSHARE_DESTINATION}/"
		DESTINATION ${SCREENSHARE_INSTALL_DIR}
	)

	set(EXECUTABLE_PATH "${SCREENSHARE_DESTINATION}/${SCREENSHARE_EXEC_NAME}")
	optional_win_executable_signing()
endif()

# DO build the Screenshare Electron app when building the `ALL_BUILD` target.
# DO build the Screenshare Electron app when a user selects "Build Solution" from within Visual Studio.
set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL FALSE EXCLUDE_FROM_DEFAULT_BUILD FALSE)
set_target_properties(${TARGET_NAME}-npm-install PROPERTIES EXCLUDE_FROM_ALL FALSE EXCLUDE_FROM_DEFAULT_BUILD FALSE)
