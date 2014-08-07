set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME shared)
project(${TARGET_NAME})

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

# include GLM
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

# link required libraries on UNIX
if (APPLE)
  find_library(CoreServices CoreServices)
  set(DEPENDENCY_LIBRARIES ${CoreServices})
elseif (UNIX)
  find_package(Threads REQUIRED)
  set(DEPENDENCY_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
endif ()

find_package(Qt5 COMPONENTS Network Widgets)

# bubble up the libraries we are dependent on and link them to ourselves
list(APPEND DEPENDENCY_LIBRARIES Qt5::Network Qt5::Widgets)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})