set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME shared)
project(${TARGET_NAME})

find_package(Qt5 COMPONENTS Network Widgets Xml Script)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

# include GLM
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

set(EXTERNAL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

if (WIN32)
    # include headers for external libraries and InterfaceConfig.
    include_directories("${EXTERNAL_ROOT_DIR}")
endif (WIN32)

# link required libraries on UNIX
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif (UNIX AND NOT APPLE)

# There is something special (bug) about Qt5Scripts, that we have to explicitly add its include
# directory when Qt5 (5.2.1) is compiled from source and is not in a standard place.
include_directories(SYSTEM "${Qt5Script_INCLUDE_DIRS}")

# bubble up the libraries we are dependent on and link them to ourselves
set(REQUIRED_DEPENDENCY_LIBRARIES Qt5::Network Qt5::Widgets)
set(SUB_DEPENDENCY_LIBRARIES ${REQUIRED_DEPENDENCY_LIBRARIES} PARENT_SCOPE)
target_link_libraries(${TARGET_NAME} ${REQUIRED_DEPENDENCY_LIBRARIES})