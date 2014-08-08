set(TARGET_NAME audio)

setup_hifi_library(${TARGET_NAME})

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} networking shared)

find_package(Qt5 COMPONENTS Network)

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES Qt5::Network)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES ${DEPENDENCY_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()
