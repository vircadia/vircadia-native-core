set(VCPKG_POLICY_EMPTY_PACKAGE enabled)
include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO vircadia/scribe
    REF 1bd638a36ca771e5a68d01985b6389b71835cbd2
    SHA512 dbe241d86df3912e544f6b9839873f9875df54efc93822b145e7b13243eaf2e3d690bc8a28b1e52d05bdcd7e68fca6b0b2f5c43ffd0f56a9b7a50d54dcf9e31e
    HEAD_REF master
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})
vcpkg_install_cmake()
vcpkg_copy_pdbs()

# cleanup
configure_file(${SOURCE_PATH}/LICENSE ${CURRENT_PACKAGES_DIR}/share/hifi-scribe/copyright COPYONLY)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/tools)

