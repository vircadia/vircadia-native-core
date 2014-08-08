set(TARGET_NAME models)

setup_hifi_library(${TARGET_NAME})

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} shared octree fbx networking animation)

find_package(Qt5 COMPONENTS Network Script)

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES Qt5::Network Qt5::Script)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})
