set(TARGET_NAME native-lib)
setup_hifi_library()
link_hifi_libraries(shared networking gl gpu gpu-gles image fbx render-utils physics)
target_opengl()
target_link_libraries(native-lib android log m)
set(GVR_ROOT "${HIFI_ANDROID_PRECOMPILED}/gvr/gvr-android-sdk-1.101.0/")
target_include_directories(native-lib PRIVATE  "${GVR_ROOT}/libraries/headers")
target_link_libraries(native-lib "${HIFI_ANDROID_PRECOMPILED}/jni/arm64-v8a/libgvr.so")

# finished libraries
# core -> qt
# networking -> openssl, tbb
# fbx -> draco
# physics -> bullet

# unfinished libraries
# image -> nvtt (doesn't look good, but can be made optional)
# script-engine -> quazip (probably not required for the android client)
# entities-renderer -> polyvox

