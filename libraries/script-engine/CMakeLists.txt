set(TARGET_NAME script-engine)

setup_hifi_library(${TARGET_NAME})

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree voxels fbx particles models animation)

find_package(Qt5 COMPONENTS Gui Network Script Widgets)

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES Qt5::Gui Qt5::Network Qt5::Script Qt5::Widgets)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})