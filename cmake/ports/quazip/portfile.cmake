include(vcpkg_common_functions)

if(EXISTS "${VCPKG_ROOT_DIR}/_env/QT_CMAKE_PREFIX_PATH.txt")
    # This environment var file only exists if we're overridding the default Qt location,
    # which happens when using Qt from vcpkg, or using Qt from custom location
    file(READ "${VCPKG_ROOT_DIR}/_env/QT_CMAKE_PREFIX_PATH.txt" QT_CMAKE_PREFIX_PATH)
    set(QUAZIP_EXTRA_OPTS "-DCMAKE_PREFIX_PATH=${QT_CMAKE_PREFIX_PATH}")
else()
    # In the case of using system Qt, don't pass anything.
    set(QUAZIP_EXTRA_OPTS "")
endif()

file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_BUILD_ASSETS.txt" EXTERNAL_BUILD_ASSETS)

vcpkg_download_distfile(
    SOURCE_ARCHIVE
    URLS ${EXTERNAL_BUILD_ASSETS}/dependencies/quazip-0.7.3.zip
    SHA512 b2d812b6346317fd6d8f4f1344ad48b721d697c429acc8b7e7cb776ce5cba15a59efd64b2c5ae1f31b5a3c928014f084aa1379fd55d8a452a6cf4fd510b3afcc
    FILENAME quazip.zip
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${SOURCE_ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS -DCMAKE_POSITION_INDEPENDENT_CODE=ON ${QUAZIP_EXTRA_OPTS} -DBUILD_WITH_QT4=OFF
)

vcpkg_install_cmake()

if (WIN32)
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
        file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/bin)
        file(RENAME ${CURRENT_PACKAGES_DIR}/lib/quazip5.dll ${CURRENT_PACKAGES_DIR}/bin/quazip5.dll)
    endif()
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
        file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/debug/bin)
        file(RENAME ${CURRENT_PACKAGES_DIR}/debug/lib/quazip5d.dll ${CURRENT_PACKAGES_DIR}/debug/bin/quazip5.dll)
    endif()
elseif(DEFINED VCPKG_TARGET_IS_LINUX)
    # We only want static libs.
    file(GLOB QUAZIP5_DYNAMIC_LIBS ${CURRENT_PACKAGES_DIR}/lib/libquazip5.so* ${CURRENT_PACKAGES_DIR}/debug/lib/libquazip5d.so*)
    file(REMOVE ${QUAZIP5_DYNAMIC_LIBS})
endif()
file(INSTALL ${SOURCE_PATH}/COPYING DESTINATION ${CURRENT_PACKAGES_DIR}/share/quazip RENAME copyright)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

# set(QUAZIP_CMAKE_ARGS 
#     -DCMAKE_PREFIX_PATH=${QT_CMAKE_PREFIX_PATH} 
#     -DCMAKE_INSTALL_NAME_DIR:PATH=<INSTALL_DIR>/lib 
#     -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
#     -DZLIB_ROOT=${VCPKG_INSTALL_ROOT}
#     -DCMAKE_POSITION_INDEPENDENT_CODE=ON)

# if (NOT APPLE)
#   set(QUAZIP_CMAKE_ARGS ${QUAZIP_CMAKE_ARGS} -DCMAKE_CXX_STANDARD=11)
# endif ()

# ExternalProject_Add(
#   ${EXTERNAL_NAME}
#   URL 
#   URL_MD5 ed03754d39b9da1775771819b8001d45
#   BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
#   CMAKE_ARGS ${QUAZIP_CMAKE_ARGS}
#   LOG_DOWNLOAD 1
#   LOG_CONFIGURE 1
#   LOG_BUILD 1
# )

# # Hide this external target (for ide users)
# set_target_properties(${EXTERNAL_NAME} PROPERTIES
#     FOLDER "hidden/externals"
#     INSTALL_NAME_DIR ${INSTALL_DIR}/lib
#     BUILD_WITH_INSTALL_RPATH True)

# ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
# set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include CACHE PATH "List of QuaZip include directories")
# set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${${EXTERNAL_NAME_UPPER}_INCLUDE_DIR} CACHE PATH "List of QuaZip include directories")
# set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${INSTALL_DIR}/lib CACHE FILEPATH "Location of QuaZip DLL")

# if (APPLE)
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libquazip5.1.0.0.dylib CACHE FILEPATH "Location of QuaZip release library")
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/libquazip5d.1.0.0.dylib CACHE FILEPATH "Location of QuaZip release library")
# elseif (WIN32)
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/quazip5.lib CACHE FILEPATH "Location of QuaZip release library")
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/quazip5d.lib CACHE FILEPATH "Location of QuaZip release library")
# elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libquazip5.so CACHE FILEPATH "Location of QuaZip release library")
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/libquazip5d.so CACHE FILEPATH "Location of QuaZip release library")
# else ()
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libquazip5.so CACHE FILEPATH "Location of QuaZip release library")
#   set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/libquazip5.so CACHE FILEPATH "Location of QuaZip release library")
# endif ()

# include(SelectLibraryConfigurations)
# select_library_configurations(${EXTERNAL_NAME_UPPER})

# # Force selected libraries into the cache
# set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE FILEPATH "Location of QuaZip libraries")
# set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Location of QuaZip libraries")
