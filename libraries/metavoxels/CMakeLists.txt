set(TARGET_NAME metavoxels)

auto_mtc(${TARGET_NAME} "${ROOT_DIR}")

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Network Script Widgets)

# link in the networking library
link_hifi_libraries(${TARGET_NAME} shared networking)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies_to_target(${TARGET_NAME})