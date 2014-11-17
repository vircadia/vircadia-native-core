set(TARGET_NAME shared)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
# TODO: We probably don't want Script and Gui to have to be dependencies here - need some decoupling for script meta types
setup_hifi_library(Network Script Gui)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()