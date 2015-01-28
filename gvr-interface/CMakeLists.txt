set(TARGET_NAME gvr-interface)

set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk-build")
set(ANDROID_APK_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}")
set(BUILD_SHARED_LIBS ON)

setup_hifi_library(Gui Widgets AndroidExtras)

include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

include_glm()

link_hifi_libraries(shared networking audio-client)
include_dependency_includes()

set(ANDROID_SDK_ROOT $ENV{ANDROID_HOME})
set(ANDROID_APP_DISPLAY_NAME Interface)
set(ANDROID_API_LEVEL 19)
set(ANDROID_APK_PACKAGE io.highfidelity.gvrinterface)
set(ANDROID_ACTIVITY_NAME io.highfidelity.gvrinterface.InterfaceActivity)
set(ANDROID_APK_VERSION_NAME "0.1")
set(ANDROID_APK_VERSION_CODE 1)
set(ANDROID_DEPLOY_QT_INSTALL "--install")

find_package(LibOVR REQUIRED)
target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES} ${LIBOVR_ANDROID_LIBRARIES})
include_directories(SYSTEM ${LIBOVR_INCLUDE_DIRS})

# we need VRLib, so add a project.properties to our apk build folder that says that
file(RELATIVE_PATH RELATIVE_VRLIB_PATH ${ANDROID_APK_OUTPUT_DIR} "${LIBOVR_VRLIB_DIR}")
file(WRITE "${ANDROID_APK_BUILD_DIR}/project.properties" "android.library.reference.1=${RELATIVE_VRLIB_PATH}")

list(APPEND IGNORE_COPY_LIBS ${LIBOVR_ANDROID_LIBRARIES})

# the presence of a HOCKEY_APP_ID means we are making a beta build
if (HOCKEY_APP_ID)
  set(HOCKEY_APP_ENABLED true)
  set(HOCKEY_APP_ACTIVITY "<activity android:name='net.hockeyapp.android.UpdateActivity' />")
  set(ANDROID_ACTIVITY_NAME io.highfidelity.gvrinterface.InterfaceBetaActivity)
  set(ANDROID_DEPLOY_QT_INSTALL "")
  
  # set the ANDROID_APK_VERSION_CODE to the number of git commits
  execute_process(
    COMMAND  git rev-list --first-parent --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_COUNT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  set(ANDROID_APK_VERSION_CODE ${GIT_COMMIT_COUNT})
  
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/InterfaceBetaActivity.java.in" "${ANDROID_APK_BUILD_DIR}/src/io/highfidelity/gvrinterface/InterfaceBetaActivity.java")
else ()
  set(HOCKEY_APP_ENABLED false)
endif ()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/hockeyapp.xml.in" "${ANDROID_APK_BUILD_DIR}/res/values/hockeyapp.xml")

qt_create_apk()