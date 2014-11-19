set(TARGET_NAME gvr-interface)

set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk-build")
set(ANDROID_APK_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}")
set(BUILD_SHARED_LIBS ON)

setup_hifi_library()

include_glm()

set(REQUIRED_HIFI_LIBRARIES "shared" "networking")

link_hifi_libraries(${REQUIRED_HIFI_LIBRARIES})
link_shared_dependencies()

set(ANDROID_SDK_ROOT $ENV{ANDROID_HOME})
set(ANDROID_APP_DISPLAY_NAME Interface)
set(ANDROID_API_LEVEL 19)
set(ANDROID_APK_PACKAGE io.highfidelity.gvrinterface)
set(ANDROID_APK_FULLSCREEN true)

# we need VRLib, so add a project.properties to our apk build folder that says that
find_path(_OCULUS_VRLIB_DIR NAME VRLib.vcxproj HINTS "/ovr_mobile_sdk/VRLib/")
if (NOT _OCULUS_VRLIB_DIR)
  message(FATAL_ERROR "Could not find Oculus Mobile SDK VRLib.")
endif()

file(RELATIVE_PATH RELATIVE_VRLIB_PATH ${ANDROID_APK_OUTPUT_DIR} "${_OCULUS_VRLIB_DIR}")
file(WRITE "${ANDROID_APK_BUILD_DIR}/project.properties" "android.library.reference.1=${RELATIVE_VRLIB_PATH}")

qt_create_apk()