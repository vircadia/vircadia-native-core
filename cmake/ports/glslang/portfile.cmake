vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO KhronosGroup/glslang
    REF "${VERSION}"
    SHA512 45ec1a23a390319b9270761cf8befb832ac8b4bc215b211c41a543553a97e5ccf17c134c34d8fdbed6efe887a9a7c2f0a955d1bfe1add9e04cc3e95b12e1973a
    HEAD_REF master
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        opt ENABLE_OPT
        opt ALLOW_EXTERNAL_SPIRV_TOOLS
        tools ENABLE_GLSLANG_BINARIES
        rtti ENABLE_RTTI
)

if (ENABLE_GLSLANG_BINARIES)
    vcpkg_find_acquire_program(PYTHON3)
    get_filename_component(PYTHON_PATH ${PYTHON3} DIRECTORY)
    vcpkg_add_to_path("${PYTHON_PATH}")
endif ()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_EXTERNAL=OFF
        -DGLSLANG_TESTS=OFF
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/glslang DO_NOT_DELETE_PARENT_CONFIG_PATH)
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/${PORT}/glslang-config.cmake"
    [[${PACKAGE_PREFIX_DIR}/lib/cmake/glslang/glslang-targets.cmake]]
    [[${CMAKE_CURRENT_LIST_DIR}/glslang-targets.cmake]]
)
file(REMOVE_RECURSE CONFIG_PATH "${CURRENT_PACKAGES_DIR}/lib/cmake" "${CURRENT_PACKAGES_DIR}/debug/lib/cmake")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/glslang/Public/ShaderLang.h" "ifdef GLSLANG_IS_SHARED_LIBRARY" "if 1")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/glslang/Include/glslang_c_interface.h" "ifdef GLSLANG_IS_SHARED_LIBRARY" "if 1")
endif()

vcpkg_copy_pdbs()

if (ENABLE_GLSLANG_BINARIES)
    vcpkg_copy_tools(TOOL_NAMES glslang spirv-remap AUTO_CLEAN)
endif ()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
