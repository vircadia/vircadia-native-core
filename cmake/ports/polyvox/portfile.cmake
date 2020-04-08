include(vcpkg_common_functions)

file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_BUILD_ASSETS.txt" EXTERNAL_BUILD_ASSETS)

# else Linux desktop
vcpkg_download_distfile(
    SOURCE_ARCHIVE
    URLS ${EXTERNAL_BUILD_ASSETS}/dependencies/polyvox-master-2015-7-15.zip
    SHA512 cc04cd43ae74b9c7bb065953540c0048053fcba6b52dc4218b3d9431fba178d65ad4f6c53cc1122ba61d0ab4061e99a7ebbb15db80011d607c5070ebebf8eddc
    FILENAME polyvox.zip
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${SOURCE_ARCHIVE}
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS -DENABLE_EXAMPLES=OFF -DENABLE_BINDINGS=OFF 
)

vcpkg_install_cmake()

file(INSTALL ${SOURCE_PATH}/LICENSE.TXT DESTINATION ${CURRENT_PACKAGES_DIR}/share/polyvox RENAME copyright)
file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/include)
if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/lib)
endif()
if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/debug/lib)
endif()
if(WIN32)
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
        file(RENAME ${CURRENT_PACKAGES_DIR}/PolyVoxCore/lib/Release/PolyVoxCore.lib ${CURRENT_PACKAGES_DIR}/lib/PolyVoxCore.lib)
        file(RENAME ${CURRENT_PACKAGES_DIR}/PolyVoxUtil/lib/PolyVoxUtil.lib ${CURRENT_PACKAGES_DIR}/lib/PolyVoxUtil.lib)
    endif()
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
        file(RENAME ${CURRENT_PACKAGES_DIR}/debug/PolyVoxCore/lib/Debug/PolyVoxCore.lib ${CURRENT_PACKAGES_DIR}/debug/lib/PolyVoxCore.lib)
        file(RENAME ${CURRENT_PACKAGES_DIR}/debug/PolyVoxUtil/lib/PolyVoxUtil.lib ${CURRENT_PACKAGES_DIR}/debug/lib/PolyVoxUtil.lib)
    endif()
    file(RENAME ${CURRENT_PACKAGES_DIR}/PolyVoxCore/include/PolyVoxCore ${CURRENT_PACKAGES_DIR}/include/PolyVoxCore)
    file(RENAME ${CURRENT_PACKAGES_DIR}/PolyVoxUtil/include/PolyVoxUtil ${CURRENT_PACKAGES_DIR}/include/PolyVoxUtil)
    file(RENAME ${CURRENT_PACKAGES_DIR}/cmake/PolyVoxConfig.cmake ${CURRENT_PACKAGES_DIR}/share/polyvox/polyvoxConfig.cmake)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/cmake)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/PolyVoxCore)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/PolyVoxUtil)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/cmake)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/PolyVoxCore)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/PolyVoxUtil)
else()
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
        file(GLOB LIBS ${CURRENT_PACKAGES_DIR}/lib/Release/*)
        foreach(_lib ${LIBS})
            file(RELATIVE_PATH _libName ${CURRENT_PACKAGES_DIR}/lib/Release ${_lib})
            file(RENAME ${_lib} ${CURRENT_PACKAGES_DIR}/lib/${_libName})
        endforeach()
    endif()
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
        file(GLOB LIBS ${CURRENT_PACKAGES_DIR}/debug/lib/Debug/*)
        foreach(_lib ${LIBS})
            file(RELATIVE_PATH _libName ${CURRENT_PACKAGES_DIR}/debug/lib/Debug ${_lib})
            file(RENAME ${_lib} ${CURRENT_PACKAGES_DIR}/debug/lib/${_libName})
        endforeach()
    endif()
    file(RENAME ${CURRENT_PACKAGES_DIR}/include/PolyVoxCore ${CURRENT_PACKAGES_DIR}/include/PolyVoxCore.temp)
    file(RENAME ${CURRENT_PACKAGES_DIR}/include/PolyVoxCore.temp/PolyVoxCore ${CURRENT_PACKAGES_DIR}/include/PolyVoxCore)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/include/PolyVoxCore.temp)
    file(RENAME ${CURRENT_PACKAGES_DIR}/include/PolyVoxUtil ${CURRENT_PACKAGES_DIR}/include/PolyVoxUtil.temp)
    file(RENAME ${CURRENT_PACKAGES_DIR}/include/PolyVoxUtil.temp/PolyVoxUtil ${CURRENT_PACKAGES_DIR}/include/PolyVoxUtil)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/include/PolyVoxUtil.temp)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/share/doc)
endif()
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/Release)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/RelWithDebInfo)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/Debug)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/Release)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/RelWithDebInfo)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/Debug)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)

# if (APPLE)
#   set(INSTALL_NAME_LIBRARY_DIR ${INSTALL_DIR}/lib)
#   ExternalProject_Add_Step(
#     ${EXTERNAL_NAME}
#     change-install-name-debug
#     COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
#     COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${INSTALL_NAME_LIBRARY_DIR}/Debug -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
#     DEPENDEES install
#     WORKING_DIRECTORY <SOURCE_DIR>
#     LOG 1
#   )
#   ExternalProject_Add_Step(
#     ${EXTERNAL_NAME}
#     change-install-name-release
#     COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
#     COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${INSTALL_NAME_LIBRARY_DIR}/Release -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
#     DEPENDEES install
#     WORKING_DIRECTORY <SOURCE_DIR>
#     LOG 1
#   )
# endif ()
