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
    REPO kasenvr/nvidia-texture-tools
    REF 330c4d56274a0f602a5c70596e2eb670a4ed56c2
    SHA512 4c0bc2f369120d696cc27710b6d33086b27eef55f537ec66b9a5c8b1839bc2426c0413670b0f65be52c5d353468f0126dfe024be1f0690611d4d7e33ac530127
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
