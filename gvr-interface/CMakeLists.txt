set(TARGET_NAME gvr-interface)

if (ANDROID)
  set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk-build")
  set(ANDROID_APK_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")

  set(ANDROID_SDK_ROOT $ENV{ANDROID_HOME})
  set(ANDROID_APP_DISPLAY_NAME Interface)
  set(ANDROID_API_LEVEL 19)
  set(ANDROID_APK_PACKAGE io.highfidelity.gvrinterface)
  set(ANDROID_ACTIVITY_NAME io.highfidelity.gvrinterface.InterfaceActivity)
  set(ANDROID_APK_VERSION_NAME "0.1")
  set(ANDROID_APK_VERSION_CODE 1)
  set(ANDROID_APK_FULLSCREEN TRUE)
  set(ANDROID_DEPLOY_QT_INSTALL "--install")

  set(BUILD_SHARED_LIBS ON)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}")

  setup_hifi_library(Gui AndroidExtras)
else ()
  setup_hifi_project(Gui)
endif ()

include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

link_hifi_libraries(shared networking audio-client avatars)

if (ANDROID)
  find_package(LibOVR)

  if (LIBOVR_FOUND)
    add_definitions(-DHAVE_LIBOVR)
    target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES} ${LIBOVR_ANDROID_LIBRARIES} ${TURBOJPEG_LIBRARY})
    include_directories(SYSTEM ${LIBOVR_INCLUDE_DIRS})

    # we need VRLib, so add a project.properties to our apk build folder that says that
    file(RELATIVE_PATH RELATIVE_VRLIB_PATH ${ANDROID_APK_OUTPUT_DIR} "${LIBOVR_VRLIB_DIR}")
    file(WRITE "${ANDROID_APK_BUILD_DIR}/project.properties" "android.library.reference.1=${RELATIVE_VRLIB_PATH}")

    list(APPEND IGNORE_COPY_LIBS ${LIBOVR_ANDROID_LIBRARIES})
  endif ()

endif ()

# the presence of a HOCKEY_APP_ID means we are making a beta build
if (ANDROID AND HOCKEY_APP_ID)
  set(HOCKEY_APP_ENABLED true)
  set(HOCKEY_APP_ACTIVITY "<activity android:name='net.hockeyapp.android.UpdateActivity' />\n")
  set(ANDROID_ACTIVITY_NAME io.highfidelity.gvrinterface.InterfaceBetaActivity)
  set(ANDROID_DEPLOY_QT_INSTALL "")
  set(ANDROID_APK_CUSTOM_NAME "Interface-beta.apk")

  # set the ANDROID_APK_VERSION_CODE to the number of git commits
  execute_process(
    COMMAND  git rev-list --first-parent --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_COUNT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(ANDROID_APK_VERSION_CODE ${GIT_COMMIT_COUNT})

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/InterfaceBetaActivity.java.in" "${ANDROID_APK_BUILD_DIR}/src/io/highfidelity/gvrinterface/InterfaceBetaActivity.java")
elseif (ANDROID)
  set(HOCKEY_APP_ENABLED false)
endif ()

if (ANDROID)

  set(HIFI_URL_INTENT "<intent-filter>\
    \n        <action android:name='android.intent.action.VIEW' />\
    \n        <category android:name='android.intent.category.DEFAULT' />\
    \n        <category android:name='android.intent.category.BROWSABLE' />\
    \n        <data android:scheme='hifi' />\
    \n      </intent-filter>"
  )

  set(ANDROID_EXTRA_APPLICATION_XML "${HOCKEY_APP_ACTIVITY}")
  set(ANDROID_EXTRA_ACTIVITY_XML "${HIFI_URL_INTENT}")

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/hockeyapp.xml.in" "${ANDROID_APK_BUILD_DIR}/res/values/hockeyapp.xml")
  qt_create_apk()

endif (ANDROID)
