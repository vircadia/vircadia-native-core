set(TARGET_NAME animation)

setup_hifi_library(${TARGET_NAME})

link_hifi_libraries(${TARGET_NAME} shared fbx)

find_package(Qt5 COMPONENTS Network Script)

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES Qt5::Script Qt5::Network)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()