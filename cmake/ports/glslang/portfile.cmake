include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO KhronosGroup/glslang
  REF untagged-048c4dbc7f021224a933
  SHA512 e3097dd2db88320982d7da1ddce138839daf3251935909c3998a114aeadd408760b26b2d7c7ee547fb0519895ac1853a45c23df2eecf61135849b080252e9dec
  HEAD_REF master
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS -DCMAKE_DEBUG_POSTFIX=d
)

vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(RENAME "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/tools")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/bin")

# Handle copyright
file(COPY ${CMAKE_CURRENT_LIST_DIR}/copyright DESTINATION ${CURRENT_PACKAGES_DIR}/share/glslang)
