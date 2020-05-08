include(vcpkg_common_functions)
set(SRANIPAL_VERSION 1.1.0.1)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src)

if (WIN32)
    vcpkg_download_distfile(
        SRANIPAL_SOURCE_ARCHIVE
        URLS https://athena-public.s3.amazonaws.com/seth/sranipal-1.1.0.1-windows.zip
        SHA512 b09ce012abe4e3c71e8e69626bdd7823ff6576601a821ab365275f2764406a3e5f7b65fcf2eb1d0962eff31eb5958a148b00901f67c229dc6ace56eb5e6c9e1b
        FILENAME sranipal-1.1.0.1-windows.zip
    )

    vcpkg_extract_source_archive(${SRANIPAL_SOURCE_ARCHIVE})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/include DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/lib DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/debug DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/bin DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/share DESTINATION ${CURRENT_PACKAGES_DIR})

endif ()
