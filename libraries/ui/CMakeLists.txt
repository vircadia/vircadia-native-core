set(TARGET_NAME ui)
setup_hifi_library(Widgets Multimedia Network Qml Quick Script WebChannel WebSockets XmlPatterns ${PLATFORM_QT_COMPONENTS})
link_hifi_libraries(shared networking gl audio audio-client plugins pointers)
include_hifi_library_headers(controllers)

# Required for some low level GL interaction in the OffscreenQMLSurface
target_opengl()
