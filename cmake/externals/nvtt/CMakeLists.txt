include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME nvtt)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (NOT ANDROID)
  if (WIN32)
    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL http://s3.amazonaws.com/hifi-public/dependencies/nvtt-win-2.1.0.zip
      URL_MD5 3ea6eeadbcc69071acf9c49ba565760e
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      LOG_DOWNLOAD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "Location of NVTT include directory")

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/Release/x64/nvtt.lib CACHE FILEPATH "Path to NVTT release library")

    set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/Release/x64" CACHE PATH "Location of NVTT release DLL")
  else ()
    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL http://hifi-public.s3.amazonaws.com/dependencies/nvidia-texture-tools-2.1.0.zip
      URL_MD5 79842ea185c8c95aaa70cee91e77f032
      CONFIGURE_COMMAND CMAKE_ARGS  ${ANDROID_CMAKE_ARGS} -DNVTT_SHARED=0 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      LOG_DOWNLOAD 1
      LOG_CONFIGURE 1
      LOG_BUILD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "Location of NVTT include directory")

    set(${EXTERNAL_NAME_UPPER}_BASE_LIBRARY ${INSTALL_DIR}/lib/static/libnvtt.a CACHE FILEPATH "Path to NVTT base static library")
    set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY ${INSTALL_DIR}/lib/static/libnvcore.a CACHE FILEPATH "Path to NVTT core static library")
    set(${EXTERNAL_NAME_UPPER}_IMAGE_LIBRARY ${INSTALL_DIR}/lib/static/libnvimage.a CACHE FILEPATH "Path to NVTT image static library")
    set(${EXTERNAL_NAME_UPPER}_MATH_LIBRARY ${INSTALL_DIR}/lib/static/libnvmath.a CACHE FILEPATH "Path to NVTT math static library")
    set(${EXTERNAL_NAME_UPPER}_THREAD_LIBRARY ${INSTALL_DIR}/lib/static/libnvthread.a CACHE FILEPATH "Path to NVTT thread static library")
  endif ()

  # Hide this external target (for IDE users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
endif()
