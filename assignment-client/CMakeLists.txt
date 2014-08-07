set(TARGET_NAME assignment-client)

set(ROOT_DIR ..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

include("${MACRO_DIR}/SetupHifiProject.cmake")
setup_hifi_project(${TARGET_NAME} TRUE)

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

# link in the shared libraries
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(audio ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(avatars ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(octree ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(voxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(fbx ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(particles ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(models ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(metavoxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(animation ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(script-engine ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(embedded-webserver ${TARGET_NAME} "${ROOT_DIR}")

if (UNIX)
  list(APPEND DEPENDENCY_LIBRARIES ${CMAKE_DL_LIBS})
endif (UNIX)

IF (WIN32)
  list(APPEND DEPENDENCY_LIBRARIES Winmm Ws2_32)
ENDIF(WIN32)

find_package(Qt5 COMPONENTS Gui Network Script Widgets)

# set a property indicating the libraries we are dependent on and link them to ourselves
list(APPEND DEPENDENCY_LIBRARIES Qt5::Gui Qt5::Network Qt5::Script Qt5::Widgets)
set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${DEPENDENCY_LIBRARIES}")
target_link_libraries(${TARGET_NAME} ${DEPENDENCY_LIBRARIES})

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()