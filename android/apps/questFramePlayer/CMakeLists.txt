set(TARGET_NAME questFramePlayer)
setup_hifi_library(AndroidExtras)
link_hifi_libraries(shared ktx shaders gpu gl oculusMobile ${PLATFORM_GL_BACKEND})
target_include_directories(${TARGET_NAME} PRIVATE ${HIFI_ANDROID_PRECOMPILED}/ovr/VrApi/Include)
target_link_libraries(${TARGET_NAME} android log m)
target_opengl()
target_oculus_mobile()


