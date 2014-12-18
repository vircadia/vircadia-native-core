set(TARGET_NAME shared)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
# TODO: there isn't really a good reason to have Script linked here - let's get what is requiring it out (RegisteredMetaTypes.cpp)
setup_hifi_library(Network Script Widgets)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()