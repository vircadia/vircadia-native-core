set(TARGET_NAME shared)
setup_hifi_library(${TARGET_NAME})

find_package(Qt5 COMPONENTS Network Widgets)

# bubble up the libraries we are dependent on and link them to ourselves
list(APPEND DEPENDENCY_LIBRARIES Qt5::Network Qt5::Widgets)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})