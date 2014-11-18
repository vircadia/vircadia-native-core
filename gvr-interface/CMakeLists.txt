set(TARGET_NAME gvr-interface)

set(${TARGET_NAME}_SHARED true)
setup_hifi_library()

include_glm()

set(REQUIRED_HIFI_LIBRARIES "shared" "networking")

link_hifi_libraries(${REQUIRED_HIFI_LIBRARIES})
link_shared_dependencies()

set(ANDROID_APP_NAME Interface)
set(ANDROID_API_LEVEL 19)
set(ANDROID_APK_PACKAGE io.highfidelity.io) 
set(ANDROID_APK_FULLSCREEN true)

qt_create_apk()