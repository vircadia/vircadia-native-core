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
  list(APPEND REQUIRED_DEPENDENCY_LIBRARIES ${CoreServices})
elseif (UNIX)
  find_package(Threads REQUIRED)
  LIST(APPEND REQUIRED_DEPENDENCY_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
endif ()

find_package(Qt5Network)
find_package(Qt5Widgets)

# bubble up the libraries we are dependent on and link them to ourselves
list(APPEND REQUIRED_DEPENDENCY_LIBRARIES Qt5::Network Qt5::Widgets)
set(SUB_DEPENDENCY_LIBRARIES ${REQUIRED_DEPENDENCY_LIBRARIES} PARENT_SCOPE)
target_link_libraries(${TARGET_NAME} ${REQUIRED_DEPENDENCY_LIBRARIES})