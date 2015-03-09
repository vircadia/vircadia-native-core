include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME LibOVR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://static.oculus.com/sdk-downloads/ovr_sdk_win_0.4.4.zip
    URL_MD5 bcc9f41346ce13bd4334a8bc94875728
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  # Different Oculus SDKs have different layouts, so we can't combine the include directories below
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/OculusSDK/LibOVR/Include ${SOURCE_DIR}/OculusSDK/LibOVR/Src  CACHE TYPE INTERNAL)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/OculusSDK/LibOVR/Lib/Win32/VS2013/libovr.lib CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${SOURCE_DIR}/OculusSDK/LibOVR/Lib/Win32/VS2013/libovrd.lib  CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_EXTRAS setupapi winmm ws2_32)

elseif(APPLE)

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://static.oculus.com/sdk-downloads/ovr_sdk_macos_0.4.4.tar.gz 
    URL_MD5 3cf8576d225798608a2aa7219b7ea11e
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/LibOVR/Include ${SOURCE_DIR}/LibOVR/Src  CACHE TYPE INTERNAL)

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/LibOVR/Lib/Mac/Release/libovr.a CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${SOURCE_DIR}/LibOVR/Lib/Mac/Debug/libovr.a  CACHE TYPE INTERNAL)

  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_EXTRAS ${COCOA_LIBRARY} ${IOKIT_LIBRARY})

elseif(NOT ANDROID)

	# http://static.oculus.com/sdk-downloads/ovr_sdk_linux_0.4.4.tar.xz
	# ec3bd8cff4a1461b4e21210e7feb0572
	ExternalProject_Add(
	  ${EXTERNAL_NAME}
	  PREFIX ${EXTERNAL_NAME}
	  GIT_REPOSITORY https://github.com/jherico/OculusSDK.git
	  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
	  LOG_DOWNLOAD ON
	)
	
endif()

select_library_configurations(${EXTERNAL_NAME_UPPER})

set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARY} ${${EXTERNAL_NAME_UPPER}_LIBRARY_EXTRAS} CACHE TYPE INTERNAL)

