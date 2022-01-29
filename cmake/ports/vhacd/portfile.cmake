include(vcpkg_common_functions)
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_VHACD_URLS.txt" EXTERNAL_VHACD_URLS)
file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_VHACD_SHA512.txt" EXTERNAL_VHACD_SHA512)

# else Linux desktop
vcpkg_download_distfile(
    SOURCE_ARCHIVE
    URLS ${EXTERNAL_VHACD_URLS}
    SHA512 ${EXTERNAL_VHACD_SHA512}
    FILENAME vhacd.zip
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${SOURCE_ARCHIVE}
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
)

vcpkg_install_cmake()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/copyright DESTINATION ${CURRENT_PACKAGES_DIR}/share/vhacd)
if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
endif()

if (WIN32)
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
        file(RENAME ${CURRENT_PACKAGES_DIR}/lib/Release/VHACD_LIB.lib ${CURRENT_PACKAGES_DIR}/lib/VHACD.lib)
        file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/Release)
    endif()
    if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
        file(RENAME ${CURRENT_PACKAGES_DIR}/debug/lib/Debug/VHACD_LIB.lib ${CURRENT_PACKAGES_DIR}/debug/lib/VHACD.lib)
        file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/Debug)
    endif()
endif()
