set(TARGET_NAME model)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()

link_hifi_libraries(shared gpu)

# call macro to link our dependencies and bubble them up via a property on our target
include_dependency_includes()
