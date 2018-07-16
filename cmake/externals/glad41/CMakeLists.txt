set(EXTERNAL_NAME glad41)

include(ExternalProject)
include(SelectLibraryConfigurations)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://public.highfidelity.com/dependencies/glad/glad41.zip
  URL_MD5 1324eeec33abe91e67d19ae551ba624d
  CONFIGURE_COMMAND CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

if (UNIX)
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
elseif (WIN32)
  set(LIB_EXT "lib")
endif ()

set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/${LIB_PREFIX}glad_d.${LIB_EXT})
set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/${LIB_PREFIX}glad.${LIB_EXT})
select_library_configurations(${EXTERNAL_NAME_UPPER})

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of glad include directories")
set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE INTERNAL "glad libraries")
