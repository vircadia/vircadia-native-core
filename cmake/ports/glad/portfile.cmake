include(vcpkg_common_functions)
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_GLAD32ES_URLS.txt" EXTERNAL_GLAD32ES_URLS)
file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_GLAD32ES_SHA512.txt" EXTERNAL_GLAD32ES_SHA512)
file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_GLAD45_URLS.txt" EXTERNAL_GLAD45_URLS)
file(READ "${VCPKG_ROOT_DIR}/_env/EXTERNAL_GLAD45_SHA512.txt" EXTERNAL_GLAD45_SHA512)
file(READ "${VCPKG_ROOT_DIR}/_env/USE_GLES.txt" USE_GLES)

# GitHub Actions Android builds fail with `FILENAME` set while desktop builds with GLES fail without a set `FILENAME`.
if (ANDROID)
    vcpkg_download_distfile(
        SOURCE_ARCHIVE
        URLS ${EXTERNAL_GLAD32ES_URLS}
        SHA512 ${EXTERNAL_GLAD32ES_SHA512}
    )
elseif (USE_GLES)
    vcpkg_download_distfile(
        SOURCE_ARCHIVE
        URLS ${EXTERNAL_GLAD32ES_URLS}
        SHA512 ${EXTERNAL_GLAD32ES_SHA512}
        FILENAME glad32es.zip
    )
else()
    # else Linux desktop
    vcpkg_download_distfile(
        SOURCE_ARCHIVE
        URLS ${EXTERNAL_GLAD45_URLS}
        SHA512 ${EXTERNAL_GLAD45_SHA512}
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
