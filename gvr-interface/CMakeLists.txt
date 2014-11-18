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

set(ANDROID_SDK_ROOT $ENV{ANDROID_SDK})
set(ANDROID_APP_NAME Interface)
set(ANDROID_API_LEVEL 19)
set(ANDROID_APK_PACKAGE io.highfidelity.gvrinterface)
set(ANDROID_APK_FULLSCREEN true)

qt_create_apk()