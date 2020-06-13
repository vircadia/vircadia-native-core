include(vcpkg_common_functions)

vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  xiph/opus
  REF
  72a3a6c13329869000b34a12ba27d8bfdfbc22b3
  SHA512
  590b852e966a497e33d129b58bc07d1205fe8fea9b158334cd8a3c7f539332ef9702bba4a37bd0be83eb5f04a218cef87645251899f099695d01c1eb8ea6e2fd
  HEAD_REF
  master)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH} PREFER_NINJA)
vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/Opus)
vcpkg_copy_pdbs()

file(INSTALL
     ${SOURCE_PATH}/COPYING
     DESTINATION
     ${CURRENT_PACKAGES_DIR}/share/opus
     RENAME copyright)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/cmake
                    ${CURRENT_PACKAGES_DIR}/lib/cmake
                    ${CURRENT_PACKAGES_DIR}/debug/include)
