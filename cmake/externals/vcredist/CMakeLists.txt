if (WIN32)
  set(EXTERNAL_NAME vcredist)
  string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

  include(ExternalProject)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL https://go.microsoft.com/fwlink/?LinkId=746572
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  set(${EXTERNAL_NAME_UPPER}_PATH ${SOURCE_DIR} CACHE FILEPATH "Location of VC Redistributable")
endif()
