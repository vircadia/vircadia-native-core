# Common Ambient Variables:
#   VCPKG_ROOT_DIR = <C:\path\to\current\vcpkg>
#   TARGET_TRIPLET is the current triplet (x86-windows, etc)
#   PORT is the current port name (zlib, etc)
#   CURRENT_BUILDTREES_DIR = ${VCPKG_ROOT_DIR}\buildtrees\${PORT}
#   CURRENT_PACKAGES_DIR  = ${VCPKG_ROOT_DIR}\packages\${PORT}_${TARGET_TRIPLET}
#
include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO vircadia/nvidia-texture-tools
    REF d8b7a98aeb177b5eddb76571183bbd2f95d54e6c
    SHA512 ea15ffd19eb1e14c8ebd62f8d7de3df1ecf6c18a339025f4a0e13419717d510903fc126ec6d1bdfbb5a2f4525a922412b72318bc8dd55dd000481a3924fbfcd4
    HEAD_REF master
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_TOOLS=OFF
)

vcpkg_install_cmake()

if(VCPKG_LIBRARY_LINKAGE STREQUAL static)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)
endif()
    
vcpkg_copy_pdbs()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)

# Handle copyright
file(REMOVE ${CURRENT_PACKAGES_DIR}/share/doc/nvtt/LICENSE)
file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/nvtt)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/nvtt/LICENSE ${CURRENT_PACKAGES_DIR}/share/nvtt/copyright)
