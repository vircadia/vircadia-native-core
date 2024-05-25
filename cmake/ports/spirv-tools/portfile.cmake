include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO KhronosGroup/SPIRV-Tools
    REF v2024.1
    SHA512 2c265a9f4b6c40501a8f1925080c0b176f5d3be50afddbbf16bcd2230ce09ca47d2393404d3717dabb7ebe25cce7decaede16223f79edfbcbf77ed9a35d74a6c
    HEAD_REF master
)

vcpkg_from_github(
    OUT_SOURCE_PATH SPIRV_HEADERS_PATH
    REPO KhronosGroup/SPIRV-Headers
    REF 801cca8104245c07e8cc53292da87ee1b76946fe
    SHA512 2bfc37beec1f6afb565fa7dad08eb838c8fe4bacda7f80a3a6c75d80c7eb1caea7e7716dc1da11b337b723a0870700d524c6617c5b7cab8b28048fa8c0785ba9
    HEAD_REF master
)

vcpkg_find_acquire_program(PYTHON3)
get_filename_component(PYTHON3_DIR "${PYTHON3}" DIRECTORY)
vcpkg_add_to_path("${PYTHON3_DIR}")

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DSPIRV-Headers_SOURCE_DIR=${SPIRV_HEADERS_PATH}
        -DSPIRV_WERROR=OFF
)

vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(GLOB EXES "${CURRENT_PACKAGES_DIR}/bin/*")
file(COPY ${EXES} DESTINATION ${CURRENT_PACKAGES_DIR}/tools)
file(REMOVE ${EXES})
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)

# Handle copyright
file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/spirv-tools)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/spirv-tools/LICENSE ${CURRENT_PACKAGES_DIR}/share/spirv-tools/copyright)
