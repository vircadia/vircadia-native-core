set(TARGET_NAME metavoxels)

auto_mtc(${TARGET_NAME} "${ROOT_DIR}")

setup_hifi_library(${TARGET_NAME} "${AUTOMTC_SRC}")

# link in the networking library
link_hifi_libraries(${TARGET_NAME} shared networking)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

find_package(Qt5 COMPONENTS Network Script Widgets)

# set a property indicating the libraries we are dependent on and link them to ourselves
set(DEPENDENCY_LIBRARIES Qt5::Script Qt5::Widgets Qt5::Network)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})