set(TARGET_NAME ui)
setup_hifi_library(OpenGL Multimedia Network Qml Quick Script WebChannel WebSockets XmlPatterns ${PLATFORM_QT_COMPONENTS})
link_hifi_libraries(shared networking qml gl audio audio-client plugins pointers)
include_hifi_library_headers(controllers)

# Required for some low level GL interaction in the OffscreenQMLSurface
set(OpenGL_GL_PREFERENCE "GLVND")
target_opengl()
