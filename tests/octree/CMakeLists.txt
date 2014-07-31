set(TARGET_NAME octree-tests)

set(ROOT_DIR ../..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

find_package(Qt5Network REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(Qt5Widgets REQUIRED)

include(${MACRO_DIR}/SetupHifiProject.cmake)
setup_hifi_project(${TARGET_NAME} TRUE)

include(${MACRO_DIR}/AutoMTC.cmake)
auto_mtc(${TARGET_NAME} ${ROOT_DIR})

qt5_use_modules(${TARGET_NAME} Network Script Widgets)

#include glm
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

# link in the shared libraries
include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(entities ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(octree ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(audio ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(networking ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(animation ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(fbx ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})

IF (WIN32)
    # add a definition for ssize_t so that windows doesn't bail
    add_definitions(-Dssize_t=long)

    #target_link_libraries(${TARGET_NAME} Winmm Ws2_32)
    target_link_libraries(${TARGET_NAME} wsock32.lib)
ENDIF(WIN32)


