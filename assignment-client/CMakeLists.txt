set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Widgets WebSockets)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${GLM_INCLUDE_DIRS})

# link in the shared libraries
link_hifi_libraries( 
  audio avatars octree environment gpu model fbx entities 
  networking animation shared script-engine embedded-webserver
  physics
)

include_application_version()

copy_dlls_beside_windows_executable()
