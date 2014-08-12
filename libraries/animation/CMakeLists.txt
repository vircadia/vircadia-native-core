set(TARGET_NAME animation)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network Script)

link_hifi_libraries(shared fbx)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()