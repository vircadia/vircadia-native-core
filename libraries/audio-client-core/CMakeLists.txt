set(TARGET_NAME audio-client-core)
if (ANDROID)
  set(PLATFORM_QT_COMPONENTS AndroidExtras)
endif ()
setup_hifi_library(Network)
link_hifi_libraries(audio)
include_hifi_library_headers(shared)
include_hifi_library_headers(networking)

if (ENABLE_WEBRTC_AUDIO)
  target_webrtc()
endif ()

# append audio includes to our list of includes to bubble
target_include_directories(${TARGET_NAME} PUBLIC "${HIFI_LIBRARY_DIR}/audio/src")

# have CMake grab externals for us
if (APPLE)
  find_library(CoreAudio CoreAudio)
  find_library(CoreFoundation CoreFoundation)
  target_link_libraries(${TARGET_NAME} ${CoreAudio} ${CoreFoundation})
endif ()
