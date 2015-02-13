set(TARGET_NAME shared)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
# TODO: there isn't really a good reason to have Script linked here - let's get what is requiring it out (RegisteredMetaTypes.cpp)
setup_hifi_library(Gui Network Script Widgets)

add_dependency_external_project(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()