set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME networking)
project(${TARGET_NAME})

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

# include GLM
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

find_package(Qt5Network)

# bubble up the libraries we are dependent on and link them to ourselves
set(REQUIRED_DEPENDENCY_LIBRARIES Qt5::Network)
set(SUB_DEPENDENCY_LIBRARIES ${REQUIRED_DEPENDENCY_LIBRARIES} PARENT_SCOPE)
target_link_libraries(${TARGET_NAME} ${REQUIRED_DEPENDENCY_LIBRARIES})

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()