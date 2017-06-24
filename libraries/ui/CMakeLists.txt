set(TARGET_NAME ui)
setup_hifi_library(OpenGL Network Qml Quick Script WebChannel WebEngine WebSockets XmlPatterns)
link_hifi_libraries(shared networking gl audio)

if (NOT ANDROID)
  # Required for some low level GL interaction in the OffscreenQMLSurface
  target_glew()
endif ()
