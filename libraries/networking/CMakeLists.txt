set(TARGET_NAME networking)

setup_hifi_library(${TARGET_NAME})

link_hifi_libraries(${TARGET_NAME} shared)

find_package(Qt5 COMPONENTS Network)

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES Qt5::Network)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})
