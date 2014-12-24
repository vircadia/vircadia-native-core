set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Widgets)

include_glm()

# link in the shared libraries
link_hifi_libraries( 
  audio avatars octree voxels gpu model fbx entities metavoxels 
  networking animation shared script-engine embedded-webserver
  physics
)

if (UNIX)
  target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif (UNIX)

include_dependency_includes()
