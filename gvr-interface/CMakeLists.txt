set(TARGET_NAME gvr-interface)

set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk-build")
set(ANDROID_APK_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}")
set(BUILD_SHARED_LIBS ON)

setup_hifi_library(Gui Widgets)

include_glm()

link_hifi_libraries(shared networking audio-client)
include_dependency_includes()

set(ANDROID_SDK_ROOT $ENV{ANDROID_HOME})
set(ANDROID_APP_DISPLAY_NAME Interface)
set(ANDROID_API_LEVEL 19)
set(ANDROID_APK_PACKAGE io.highfidelity.gvrinterface)

find_package(LibOVR REQUIRED)
target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})
include_directories(SYSTEM ${LIBOVR_INCLUDE_DIRS})

# we need VRLib, so add a project.properties to our apk build folder that says that
file(RELATIVE_PATH RELATIVE_VRLIB_PATH ${ANDROID_APK_OUTPUT_DIR} "${LIBOVR_VRLIB_DIR}")
file(WRITE "${ANDROID_APK_BUILD_DIR}/project.properties" "android.library.reference.1=${RELATIVE_VRLIB_PATH}")

list(APPEND IGNORE_COPY_LIBS ${LIBOVR_LIBRARIES})

qt_create_apk()