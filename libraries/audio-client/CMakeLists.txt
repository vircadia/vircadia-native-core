set(TARGET_NAME audio-client)
setup_hifi_library(Network Multimedia)
link_hifi_libraries(audio plugins)
include_hifi_library_headers(shared)
include_hifi_library_headers(networking)

# append audio includes to our list of includes to bubble
target_include_directories(${TARGET_NAME} PUBLIC "${HIFI_LIBRARY_DIR}/audio/src")

# have CMake grab externals for us
if (APPLE)
  find_library(CoreAudio CoreAudio)
  find_library(CoreFoundation CoreFoundation)
  target_link_libraries(${TARGET_NAME} ${CoreAudio} ${CoreFoundation})
endif ()
