set(TARGET_NAME assignment-client)

setup_hifi_project(${TARGET_NAME} TRUE)

include_glm(${TARGET_NAME})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} 
  audio avatars octree voxels fbx particles models metavoxels 
  networking animation shared script-engine embedded-webserver
)

if (UNIX)
  target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES} ${CMAKE_DL_LIBS})
endif (UNIX)

find_package(Qt5 COMPONENTS Gui Network Script Widgets)
target_link_libraries(${TARGET_NAME} Qt5::Gui Qt5::Network Qt5::Script Qt5::Widgets)
