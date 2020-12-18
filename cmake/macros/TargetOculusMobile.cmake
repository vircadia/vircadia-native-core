
macro(target_oculus_mobile)
    set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/ovr_sdk_mobile_1.37.0/VrApi)

    # Mobile SDK
    set(OVR_MOBILE_INCLUDE_DIRS ${INSTALL_DIR}/Include)
    target_include_directories(${TARGET_NAME} PRIVATE ${OVR_MOBILE_INCLUDE_DIRS})
    set(OVR_MOBILE_LIBRARY_DIR  ${INSTALL_DIR}/Libs/Android/arm64-v8a)
    set(OVR_MOBILE_LIBRARY_RELEASE ${OVR_MOBILE_LIBRARY_DIR}/Release/libvrapi.so)
    set(OVR_MOBILE_LIBRARY_DEBUG ${OVR_MOBILE_LIBRARY_DIR}/Debug/libvrapi.so)
    select_library_configurations(OVR_MOBILE)
    target_link_libraries(${TARGET_NAME} ${OVR_MOBILE_LIBRARIES})

    # Platform SDK
    set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/ovr_platform_sdk_23.0.0)
    set(OVR_PLATFORM_INCLUDE_DIRS ${INSTALL_DIR}/Include)
    target_include_directories(${TARGET_NAME} PRIVATE ${OVR_PLATFORM_INCLUDE_DIRS})
    set(OVR_PLATFORM_LIBRARIES ${INSTALL_DIR}/Android/libs/arm64-v8a/libovrplatformloader.so)
    target_link_libraries(${TARGET_NAME} ${OVR_PLATFORM_LIBRARIES})
endmacro()
