set(EXTERNAL_NAME quazip)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
cmake_policy(SET CMP0046 OLD)

include(ExternalProject)

if (WIN32)
    # windows shell does not like backslashes expanded on the command line,
    # so convert all backslashes in the QT path to forward slashes
    string(REPLACE \\ / QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
elseif ($ENV{QT_CMAKE_PREFIX_PATH})
    set(QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
endif ()

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://s3-us-west-1.amazonaws.com/hifi-production/dependencies/quazip-0.7.2.zip
  URL_MD5 2955176048a31262c09259ca8d309d19
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=${QT_CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_NAME_DIR:PATH=<INSTALL_DIR>/lib -DZLIB_ROOT=${ZLIB_ROOT} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

add_dependencies(quazip zlib)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES 
	FOLDER "hidden/externals"
	INSTALL_NAME_DIR ${INSTALL_DIR}/lib
	BUILD_WITH_INSTALL_RPATH True)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include CACHE PATH "List of QuaZip include directories")
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${${EXTERNAL_NAME_UPPER}_INCLUDE_DIR} CACHE PATH "List of QuaZip include directories")
set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${INSTALL_DIR}/lib CACHE FILEPATH "Location of QuaZip DLL")

if (APPLE)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libquazip5.1.0.0.dylib CACHE FILEPATH "Location of QuaZip release library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/libquazip5d.1.0.0.dylib CACHE FILEPATH "Location of QuaZip release library")
elseif (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/quazip5.lib CACHE FILEPATH "Location of QuaZip release library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/quazip5d.lib CACHE FILEPATH "Location of QuaZip release library")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libquazip5.so CACHE FILEPATH "Location of QuaZip release library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/libquazip5d.so CACHE FILEPATH "Location of QuaZip release library")
endif ()

include(SelectLibraryConfigurations)
select_library_configurations(${EXTERNAL_NAME_UPPER})

# Force selected libraries into the cache
set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE FILEPATH "Location of QuaZip libraries")
set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Location of QuaZip libraries") 