set(TARGET_NAME audio)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Network)

include_glm(${TARGET_NAME} "${ROOT_DIR}")

link_hifi_libraries(${TARGET_NAME} networking shared)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies_to_target(${TARGET_NAME})