set(TARGET_NAME physics)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()

link_hifi_libraries(shared)

## find BULLET
#find_package(BULLET REQUIRED)
#
#include_directories(SYSTEM "${BULLET_INCLUDE_DIRS}")
#
## append BULLET to our list of libraries to link
#list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${BULLET_LIBRARIES}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
