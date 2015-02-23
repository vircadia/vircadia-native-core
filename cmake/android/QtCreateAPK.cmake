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
  
	if (UPPER_CMAKE_BUILD_TYPE MATCHES RELEASE)
		set(ANDROID_APK_DEBUGGABLE "false")
		set(ANDROID_APK_RELEASE_LOCAL ${ANDROID_APK_RELEASE})
	else ()
		set(ANDROID_APK_DEBUGGABLE "true")
		set(ANDROID_APK_RELEASE_LOCAL "0")
	endif ()
  
	# Create "AndroidManifest.xml"
	configure_file("${ANDROID_THIS_DIRECTORY}/AndroidManifest.xml.in" "${ANDROID_APK_BUILD_DIR}/AndroidManifest.xml")
  
  # create "strings.xml"
  configure_file("${ANDROID_THIS_DIRECTORY}/strings.xml.in" "${ANDROID_APK_BUILD_DIR}/res/values/strings.xml")
  
  # find androiddeployqt
  find_program(ANDROID_DEPLOY_QT androiddeployqt HINTS "${QT_DIR}/bin")
  
  # set the path to our app shared library
  set(EXECUTABLE_DESTINATION_PATH "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}/lib${TARGET_NAME}.so")
  
  # add our dependencies to the deployment file
  get_property(_DEPENDENCIES TARGET ${TARGET_NAME} PROPERTY INTERFACE_LINK_LIBRARIES)
  
  foreach(_IGNORE_COPY IN LISTS IGNORE_COPY_LIBS)
    list(REMOVE_ITEM _DEPENDENCIES ${_IGNORE_COPY})
  endforeach()
  
  foreach(_DEP IN LISTS _DEPENDENCIES)
    if (NOT TARGET ${_DEP})
      list(APPEND _DEPS_LIST ${_DEP})
    else ()
      if(NOT _DEP MATCHES "Qt5::.*")
        get_property(_DEP_LOCATION TARGET ${_DEP} PROPERTY "LOCATION_${CMAKE_BUILD_TYPE}")
        
        # recurisvely add libraries which are dependencies of this target
        get_property(_DEP_DEPENDENCIES TARGET ${_DEP} PROPERTY INTERFACE_LINK_LIBRARIES)
      
        foreach(_SUB_DEP IN LISTS _DEP_DEPENDENCIES)
          if (NOT TARGET ${_SUB_DEP} AND NOT _SUB_DEP MATCHES "Qt5::.*")
            list(APPEND _DEPS_LIST ${_SUB_DEP})
          endif()
        endforeach()
        
        list(APPEND _DEPS_LIST ${_DEP_LOCATION})
      endif()
    endif ()
  endforeach()
  
  list(REMOVE_DUPLICATES _DEPS_LIST)
  
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
  
  # copy the res folder from the target to the apk build dir
  add_custom_target(
    ${TARGET_NAME}-copy-res
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/res" "${ANDROID_APK_BUILD_DIR}/res"
  )
  
  # copy the assets folder from the target to the apk build dir
  add_custom_target(
    ${TARGET_NAME}-copy-assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${ANDROID_APK_BUILD_DIR}/assets"
  )
  
  # copy the java folder from src to the apk build dir
  add_custom_target(
    ${TARGET_NAME}-copy-java
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/java" "${ANDROID_APK_BUILD_DIR}/src"
  )
  
  # copy the libs folder from src to the apk build dir
  add_custom_target(
    ${TARGET_NAME}-copy-libs
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/libs" "${ANDROID_APK_BUILD_DIR}/libs"
  )
  
  # handle setup for ndk-gdb
  add_custom_target(${TARGET_NAME}-gdb DEPENDS ${TARGET_NAME})
  
  if (ANDROID_APK_DEBUGGABLE)
    get_property(TARGET_LOCATION TARGET ${TARGET_NAME} PROPERTY LOCATION)
  
    set(GDB_SOLIB_PATH ${ANDROID_APK_BUILD_DIR}/obj/local/${ANDROID_NDK_ABI_NAME}/)

    # generate essential Android Makefiles
    file(WRITE ${ANDROID_APK_BUILD_DIR}/jni/Android.mk "APP_ABI := ${ANDROID_NDK_ABI_NAME}\n")
    file(WRITE ${ANDROID_APK_BUILD_DIR}/jni/Application.mk "APP_ABI := ${ANDROID_NDK_ABI_NAME}\n")
  
    # create gdb.setup
    get_directory_property(PROJECT_INCLUDES DIRECTORY ${PROJECT_SOURCE_DIR} INCLUDE_DIRECTORIES)
    string(REGEX REPLACE ";" " " PROJECT_INCLUDES "${PROJECT_INCLUDES}")
    file(WRITE ${ANDROID_APK_BUILD_DIR}/libs/${ANDROID_NDK_ABI_NAME}/gdb.setup "set solib-search-path ${GDB_SOLIB_PATH}\n")
    file(APPEND ${ANDROID_APK_BUILD_DIR}/libs/${ANDROID_NDK_ABI_NAME}/gdb.setup "directory ${PROJECT_INCLUDES}\n")
  
    # copy lib to obj
    add_custom_command(TARGET ${TARGET_NAME}-gdb PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${GDB_SOLIB_PATH})
    add_custom_command(TARGET ${TARGET_NAME}-gdb PRE_BUILD COMMAND cp ${TARGET_LOCATION} ${GDB_SOLIB_PATH})
  
    # strip symbols
    add_custom_command(TARGET ${TARGET_NAME}-gdb PRE_BUILD COMMAND ${CMAKE_STRIP} ${TARGET_LOCATION})
  endif () 
  
  # use androiddeployqt to create the apk
  add_custom_target(${TARGET_NAME}-apk
    COMMAND ${ANDROID_DEPLOY_QT} --input "${TARGET_NAME}-deployment.json" --output "${ANDROID_APK_OUTPUT_DIR}" --android-platform android-${ANDROID_API_LEVEL} ${ANDROID_DEPLOY_QT_INSTALL} --verbose --deployment bundled "\\$(ARGS)"
    DEPENDS ${TARGET_NAME} ${TARGET_NAME}-copy-res ${TARGET_NAME}-copy-assets ${TARGET_NAME}-copy-java ${TARGET_NAME}-copy-libs ${TARGET_NAME}-gdb
  )
  
  # rename the APK if the caller asked us to
  if (ANDROID_APK_CUSTOM_NAME) 
    add_custom_command(
      TARGET ${TARGET_NAME}-apk
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rename "${ANDROID_APK_OUTPUT_DIR}/bin/QtApp-debug.apk" "${ANDROID_APK_OUTPUT_DIR}/bin/${ANDROID_APK_CUSTOM_NAME}"
    )
  endif ()
endmacro()