set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME networking)
project(${TARGET_NAME})

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

# include GLM
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

find_package(Qt5Network REQUIRED)
set(DEPENDENCY_QT_MODULES Network)
set(REQUIRED_DEPENDENCY_QT_MODULES ${DEPENDENCY_QT_MODULES} PARENT_SCOPE)
qt5_use_modules(${TARGET_NAME} ${DEPENDENCY_QT_MODULES})

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()