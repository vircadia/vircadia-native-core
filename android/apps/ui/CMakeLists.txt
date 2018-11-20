set(TARGET_NAME uiApp)
setup_hifi_library(AndroidExtras)
link_hifi_libraries(shared ktx shaders gl oculusMobile qml)
target_link_libraries(${TARGET_NAME} android log m)
target_opengl()
target_oculus_mobile()

