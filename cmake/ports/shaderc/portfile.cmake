include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/shaderc
    REF v2018.0
    SHA512 7a420fde73c9f2aae3f13558d538a1f4ae43bba19e2b4d2da8fbbd017e9e4f328ece5f330f1bbcb9fe84c91b7eb84b9158dc2e3d144c82939090a0fa6f5b4ef0
    HEAD_REF master
    PATCHES
        0001-Do-not-generate-build-version.inc.patch
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH}/third_party/glslang)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists_spirv.txt DESTINATION ${SOURCE_PATH}/third_party/spirv-tools)
file(RENAME ${SOURCE_PATH}/third_party/spirv-tools/CMakeLists_spirv.txt ${SOURCE_PATH}/third_party/spirv-tools/CMakeLists.txt)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/build-version.inc DESTINATION ${SOURCE_PATH}/glslc/src)

#Note: glslang and spir tools doesn't export symbol and need to be build as static lib for cmake to work
set(VCPKG_LIBRARY_LINKAGE "static")
set(OPTIONS)
if(VCPKG_CRT_LINKAGE STREQUAL "dynamic")
    list(APPEND OPTIONS -DSHADERC_ENABLE_SHARED_CRT=ON)
endif()

# shaderc uses python to manipulate copyright information
vcpkg_find_acquire_program(PYTHON3)
get_filename_component(PYTHON3_EXE_PATH ${PYTHON3} DIRECTORY)
vcpkg_add_to_path(PREPEND "${PYTHON3_EXE_PATH}")

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS -DSHADERC_SKIP_TESTS=true ${OPTIONS} -Dglslang_SOURCE_DIR=${CURRENT_INSTALLED_DIR}/include
    OPTIONS_DEBUG -DSUFFIX_D=true
    OPTIONS_RELEASE -DSUFFIX_D=false
)

vcpkg_install_cmake()

file(GLOB EXES "${CURRENT_PACKAGES_DIR}/bin/*.exe")
file(COPY ${EXES} DESTINATION ${CURRENT_PACKAGES_DIR}/tools)

#Safe to remove as libs are static
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/bin)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

# Handle copyright
file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/shaderc)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/shaderc/LICENSE ${CURRENT_PACKAGES_DIR}/share/shaderc/copyright)
