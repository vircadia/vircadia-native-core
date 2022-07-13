include(vcpkg_common_functions)

set(VERSION 1.2.11)

file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_ZLIB_URLS.txt" EXTERNAL_ZLIB_URLS)
file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_ZLIB_SHA512.txt" EXTERNAL_ZLIB_SHA512)

vcpkg_download_distfile(ARCHIVE_FILE
    URLS ${EXTERNAL_ZLIB_URLS}
    FILENAME "zlib1211.tar.gz"
    SHA512 ${EXTERNAL_ZLIB_SHA512}
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE_FILE}
    REF ${VERSION}
    PATCHES
        "cmake_dont_build_more_than_needed.patch"
)

# This is generated during the cmake build
file(REMOVE ${SOURCE_PATH}/zconf.h)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DSKIP_INSTALL_FILES=ON
        -DSKIP_BUILD_EXAMPLES=ON
    OPTIONS_DEBUG
        -DSKIP_INSTALL_HEADERS=ON
)

vcpkg_install_cmake()

# Both dynamic and static are built, so keep only the one needed
if(VCPKG_LIBRARY_LINKAGE STREQUAL static)
    if(EXISTS ${CURRENT_PACKAGES_DIR}/lib/zlibstatic.lib)
        file(RENAME ${CURRENT_PACKAGES_DIR}/lib/zlibstatic.lib ${CURRENT_PACKAGES_DIR}/lib/zlib.lib)
    endif()
    if(EXISTS ${CURRENT_PACKAGES_DIR}/debug/lib/zlibstaticd.lib)
        file(RENAME ${CURRENT_PACKAGES_DIR}/debug/lib/zlibstaticd.lib ${CURRENT_PACKAGES_DIR}/debug/lib/zlibd.lib)
    endif()
endif()

file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/zlib RENAME copyright)

vcpkg_copy_pdbs()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})

vcpkg_test_cmake(PACKAGE_NAME ZLIB MODULE)
