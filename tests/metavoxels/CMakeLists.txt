set(TARGET_NAME metavoxel-tests)

find_package(Qt5 COMPONENTS Network Script Widgets)

auto_mtc(${TARGET_NAME} "${ROOT_DIR}")

setup_hifi_project(${TARGET_NAME} TRUE "${AUTOMTC_SRC}")

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} metavoxels networking shared)

target_link_libraries(${TARGET_NAME} Qt5::Network Qt5::Widgets Qt5::Script)

