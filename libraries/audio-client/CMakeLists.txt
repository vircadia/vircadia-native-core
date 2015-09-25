set(TARGET_NAME audio-client)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network Multimedia)

link_hifi_libraries(audio)

# append audio includes to our list of includes to bubble
target_include_directories(${TARGET_NAME} PUBLIC "${HIFI_LIBRARY_DIR}/audio/src")

# have CMake grab externals for us
add_dependency_external_projects(gverb)

find_package(Gverb REQUIRED)

target_link_libraries(${TARGET_NAME} ${GVERB_LIBRARIES})
target_include_directories(${TARGET_NAME} PRIVATE ${GVERB_INCLUDE_DIRS})

if (APPLE)
  find_library(CoreAudio CoreAudio)
  find_library(CoreFoundation CoreFoundation)
  target_link_libraries(${TARGET_NAME} ${CoreAudio} ${CoreFoundation})
endif ()
