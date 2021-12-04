set(TARGET_NAME qml)
setup_hifi_library(Multimedia Network Qml Quick WebChannel WebSockets ${PLATFORM_QT_COMPONENTS})
link_hifi_libraries(shared networking gl)

# Required for some low level GL interaction in the OffscreenQMLSurface
set(OpenGL_GL_PREFERENCE "GLVND")
target_opengl()
