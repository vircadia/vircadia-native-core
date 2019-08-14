set(TARGET_NAME platform)

setup_hifi_library()
link_hifi_libraries(shared gl)

GroupSources("src")
target_json()

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)
  find_library(AppKit AppKit)

  target_link_libraries(${TARGET_NAME} ${OpenGL} ${AppKit})

elseif(WIN32)
    target_link_libraries(${TARGET_NAME} Iphlpapi.lib)
endif()