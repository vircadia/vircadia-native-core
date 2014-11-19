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
set(ANDROID_THIS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})	# Directory this CMake file is in

if (POLICY CMP0026)
  cmake_policy(SET CMP0026 OLD)
endif () 

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
	configure_file("${ANDROID_THIS_DIRECTORY}/AndroidManifest.xml.in" "${ANDROID_APK_BUILD_DIR}/AndroidManifest.xml")
  
  # create "strings.xml"
  configure_file("${ANDROID_THIS_DIRECTORY}/strings.xml.in" "${ANDROID_APK_BUILD_DIR}/res/values/strings.xml")
  
  # copy the res folder from the target to the apk build dir
  add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/res" "${ANDROID_APK_BUILD_DIR}/res"
  )
  
  # copy the assets folder from the target to the apk build dir
  add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${ANDROID_APK_BUILD_DIR}/assets"
  )

  # figure out where the qt dir is
  get_filename_component(QT_DIR "${QT_CMAKE_PREFIX_PATH}/../../" ABSOLUTE)
  
  # find androiddeployqt
  find_program(ANDROID_DEPLOY_QT androiddeployqt HINTS "${QT_DIR}/bin")
  
  # set the path to our app shared library
  set(EXECUTABLE_DESTINATION_PATH "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}/lib${TARGET_NAME}.so")
  
  # add our dependencies to the deployment file
  get_property(_DEPENDENCIES TARGET ${TARGET_NAME} PROPERTY INTERFACE_LINK_LIBRARIES)
  
  foreach(_DEP IN LISTS _DEPENDENCIES)
    if (NOT TARGET ${_DEP})
      list(APPEND _DEPS_LIST ${_DEP})
    else ()
      if(NOT _DEP MATCHES "Qt5::.*")
        get_property(_DEP_LOCATION TARGET ${_DEP} PROPERTY "LOCATION_${CMAKE_BUILD_TYPE}")
        list(APPEND _DEPS_LIST ${_DEP_LOCATION})
      endif()
    endif ()
  endforeach()
  
  # just copy static libs to apk libs folder - don't add to deps list
  foreach(_LOCATED_DEP IN LISTS _DEPS_LIST)
    if (_LOCATED_DEP MATCHES "\\.a$")
      add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
	      COMMAND ${CMAKE_COMMAND} -E copy ${_LOCATED_DEP} "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}"
		  )
      list(REMOVE_ITEM _DEPS_LIST ${_LOCATED_DEP})
    endif ()
  endforeach()
  
  string(REPLACE ";" "," _DEPS "${_DEPS_LIST}")
  
  configure_file("${ANDROID_THIS_DIRECTORY}/deployment-file.json.in" "${TARGET_NAME}-deployment.json")
  
  # use androiddeployqt to create the apk
  add_custom_target(${TARGET_NAME}-apk
    COMMAND ${ANDROID_DEPLOY_QT} --input "${TARGET_NAME}-deployment.json" --output "${ANDROID_APK_OUTPUT_DIR}" --android-platform android-${ANDROID_API_LEVEL} --install --deployment bundled "\\$(ARGS)"
    DEPENDS ${TARGET_NAME}
  )
endmacro()