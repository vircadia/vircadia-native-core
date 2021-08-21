include(vcpkg_common_functions)
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_BUILD_ASSETS.txt" EXTERNAL_BUILD_ASSETS)
file(READ "${VCPKG_ROOT_DIR}/_env/USE_GLES.txt" USE_GLES)

# GitHub Actions Android builds fail with `FILENAME` set while desktop builds with GLES fail without a set `FILENAME`.
if (ANDROID)
    vcpkg_download_distfile(
        SOURCE_ARCHIVE
        URLS ${EXTERNAL_BUILD_ASSETS}/dependencies/glad/glad32es.zip
        SHA512 2e02ac633eed8f2ba2adbf96ea85d08998f48dd2e9ec9a88ec3c25f48eaf1405371d258066327c783772fcb3793bdb82bd7375fdabb2ba5e2ce0835468b17f65
    )
elseif (USE_GLES)
    vcpkg_download_distfile(
        SOURCE_ARCHIVE
        URLS ${EXTERNAL_BUILD_ASSETS}/dependencies/glad/glad32es.zip
        SHA512 2e02ac633eed8f2ba2adbf96ea85d08998f48dd2e9ec9a88ec3c25f48eaf1405371d258066327c783772fcb3793bdb82bd7375fdabb2ba5e2ce0835468b17f65
        FILENAME glad32es.zip
    )
else()
    # else Linux desktop
    vcpkg_download_distfile(
        SOURCE_ARCHIVE
        URLS ${EXTERNAL_BUILD_ASSETS}/dependencies/glad/glad45.zip
        SHA512 653a7b873f9fbc52e0ab95006cc3143bc7b6f62c6e032bc994e87669273468f37978525c9af5efe36f924cb4acd221eb664ad9af0ce4bf711b4f1be724c0065e
        FILENAME glad45.zip
    )
endif()

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${SOURCE_ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
)

vcpkg_install_cmake()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/copyright DESTINATION ${CURRENT_PACKAGES_DIR}/share/glad)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
