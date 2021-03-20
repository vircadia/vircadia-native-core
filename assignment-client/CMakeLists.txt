set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Quick WebSockets)

# Fix up the rpath so macdeployqt works
if (APPLE)
  set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
endif ()

setup_memory_debugger()

# link in the shared libraries
link_hifi_libraries(
  audio avatars octree gpu graphics shaders fbx hfm entities
  networking animation recording shared script-engine embedded-webserver
  controllers physics plugins midi image
  material-networking model-networking ktx shaders
)
include_hifi_library_headers(procedural)

if (WIN32)
  package_libraries_for_deployment()
endif()

install_beside_console()
