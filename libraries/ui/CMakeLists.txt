set(TARGET_NAME ui)
setup_hifi_library(OpenGL Network Qml Quick Script WebChannel WebEngine WebSockets XmlPatterns)
link_hifi_libraries(shared networking gl audio pointers)
include_hifi_library_headers(controllers)

# Required for some low level GL interaction in the OffscreenQMLSurface
target_opengl()
