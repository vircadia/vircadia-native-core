set(TARGET_NAME assignment-client)

setup_hifi_project(${TARGET_NAME} Core Gui Network Script Widgets)

include_glm(${TARGET_NAME})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} 
  audio avatars octree voxels fbx particles models metavoxels 
  networking animation shared script-engine embedded-webserver
)

if (UNIX)
  list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK ${CMAKE_DL_LIBS})
endif (UNIX)

link_shared_dependencies_to_target(${TARGET_NAME})