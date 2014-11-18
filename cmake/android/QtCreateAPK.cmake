# 
#  QtCreateAPK.cmake
# 
#  Created by Stephen Birarda on 11/18/14.
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# 
# OPTIONS
# These options will modify how QtCreateAPK behaves. May be useful if somebody wants to fork.
# For High Fidelity purposes these should not need to be changed.
# 
set(ANDROID_APK_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/apk")


set(ANDROID_THIS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})	# Directory this CMake file is in

macro(qt_create_apk)
	if(ANDROID_APK_FULLSCREEN)
		set(ANDROID_APK_THEME "android:theme=\"@android:style/Theme.NoTitleBar.Fullscreen\"")
	else()
		set(ANDROID_APK_THEME "")
	endif()
  
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		set(ANDROID_APK_DEBUGGABLE "true")
		set(ANDROID_APK_RELEASE_LOCAL "0")
	else ()
		set(ANDROID_APK_DEBUGGABLE "false")
		set(ANDROID_APK_RELEASE_LOCAL ${ANDROID_APK_RELEASE})
	endif ()
  
	# Create "AndroidManifest.xml"
	configure_file("${ANDROID_THIS_DIRECTORY}/AndroidManifest.xml.in" "${ANDROID_APK_DIRECTORY}/AndroidManifest.xml")

	# Create "res/values/strings.xml"
	configure_file("${ANDROID_THIS_DIRECTORY}/strings.xml.in" "${ANDROID_APK_DIRECTORY}/res/values/strings.xml")

  # figure out where the qt dir is
  get_filename_component(_QT_DIR "${QT_CMAKE_PREFIX_PATH}/../../../" ABSOLUTE)
  
  # find androiddeployqt
  find_program(ANDROID_DEPLOY_QT androiddeployqt HINTS "${_QT_DIR}/bin")
  
  # set the path to our app shared library
  set(EXECUTABLE_DESTINATION_PATH "${CMAKE_BINARY_DIR}/libs/${ANDROID_ABI}/lib${TARGET_NAME}.so")
  
  # add our dependencies to the deployment file
  get_property(_DEPENDENCIES TARGET ${TARGET_NAME} PROPERTY INTERFACE_LINK_LIBRARIES)
  set(_DEPS_LIST)
  message(${_DEPENDENCIES})
  foreach(_DEP IN LISTS _DEPENDENCIES)
      if(NOT _DEP MATCHES "Qt5::.*")
          get_property(_DEP_LOCATION TARGET ${_DEP} PROPERTY "LOCATION_${CMAKE_BUILD_TYPE}")
          list(APPEND _DEPS_LIST ${_DEP_LOCATION})
      endif()
  endforeach()
  string(REPLACE ";" "," _DEPS "${_DEPS_LIST}")
  configure_file("${ANDROID_THIS_DIRECTORY}/deployment-file.json.in" "${TARGET_NAME}-deployment.json")
  
  # Uninstall previous version from the device/emulator (else we may get e.g. signature conflicts)
  add_custom_command(TARGET ${TARGET_NAME}
  	COMMAND adb uninstall ${ANDROID_APK_PACKAGE}
  )  
endmacro()