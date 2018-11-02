set(EXTERNAL_NAME glslang)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/KhronosGroup/glslang/archive/7.8.2853.zip
  URL_MD5 4f93e3818528176c622c137fba05cbf8
  CMAKE_ARGS  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>-$<CONFIG> 
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

# includes
ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(SUFFIXED_INSTALL_DIR "${INSTALL_DIR}-$<CONFIG>")

list(APPEND INCLUDE_DIRS ${SUFFIXED_INSTALL_DIR}/include)
#list(APPEND INCLUDE_DIRS ${INSTALL_DIR}/include)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INCLUDE_DIRS} CACHE PATH "List of glslang include directories")
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${${EXTERNAL_NAME_UPPER}_INCLUDE_DIR} CACHE PATH "List of glslang include directories")


set(LIB_DIR ${SUFFIXED_INSTALL_DIR}/lib)
list(APPEND LIB_NAMES glslang HLSL OGLCompiler OSDependent SPIRV SPVRemapper)
include(SelectLibraryConfigurations)

foreach(BASE_LIB ${LIB_NAMES})
    string(TOUPPER ${BASE_LIB} BASE_LIB_UPPER)
    list(APPEND ${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE "${LIB_DIR}/${BASE_LIB}.lib")
    list(APPEND ${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "${LIB_DIR}/${BASE_LIB}d.lib")
endforeach()

select_library_configurations(${EXTERNAL_NAME_UPPER})

set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE FILEPATH "Location of glslang libraries")
set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Location of glslang libraries")
