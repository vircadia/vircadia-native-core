set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Widgets WebSockets)

# link in the shared libraries
link_hifi_libraries( 
  audio avatars octree environment gpu model fbx entities 
  networking animation shared script-engine embedded-webserver
  physics
)

include_application_version()

copy_dlls_beside_windows_executable()
