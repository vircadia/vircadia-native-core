set(TARGET_NAME audio)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network)

include_glm()

link_hifi_libraries(networking shared)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()