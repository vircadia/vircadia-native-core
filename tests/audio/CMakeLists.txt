set(TARGET_NAME audio-tests)

setup_hifi_project(${TARGET_NAME})

#include glm
include_glm(${TARGET_NAME} ${ROOT_DIR})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} shared audio networking)