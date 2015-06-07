set(TARGET_NAME shared)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
# TODO: there isn't really a good reason to have Script linked here - let's get what is requiring it out (RegisteredMetaTypes.cpp)
setup_hifi_library(Gui Network OpenGL Script Widgets)

setup_hifi_opengl()

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(boostconfig)
find_package(BOOSTCONFIG REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${BOOSTCONFIG_INCLUDE_DIRS})

add_dependency_external_projects(oglplus)
find_package(OGLPLUS REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${OGLPLUS_INCLUDE_DIRS})
