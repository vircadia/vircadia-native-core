set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Widgets)

include_glm()

# link in the shared libraries
link_hifi_libraries( 
  audio avatars octree voxels fbx entities metavoxels 
  networking animation shared script-engine embedded-webserver
)

if (UNIX)
  list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK ${CMAKE_DL_LIBS})
endif (UNIX)

link_shared_dependencies()
