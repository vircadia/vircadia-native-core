set(ROOT_DIR ../..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules/")

set(TARGET_NAME octree)

include(${MACRO_DIR}/SetupHifiLibrary.cmake)
setup_hifi_library(${TARGET_NAME})

include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

include(${MACRO_DIR}/LinkHifiLibrary.cmake)
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")

# link ZLIB
find_package(ZLIB)
find_package(OpenSSL REQUIRED)

include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}" "${OPENSSL_INCLUDE_DIR}")

# bubble up the libraries we are dependent on
set(REQUIRED_DEPENDENCY_LIBRARIES ${REQUIRED_DEPENDENCY_LIBRARIES} 
  "${ZLIB_LIBRARIES}" "${OPENSSL_LIBRARIES}" PARENT_SCOPE)
  
target_link_libraries(${TARGET_NAME} ${REQUIRED_DEPENDENCY_LIBRARIES})

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()