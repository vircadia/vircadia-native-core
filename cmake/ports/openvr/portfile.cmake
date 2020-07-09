include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ValveSoftware/openvr
    REF v1.11.11
    SHA512 25bddb0e82eea091fe5101d0d3de1de7bb81b4504adc0c8d8e687d2502c0167bc5a11e68bc343d7563fb4db7c917e9d0e2ea99bc1d8016d479874b0c6bd7f121
    HEAD_REF master
)

set(VCPKG_LIBRARY_LINKAGE dynamic)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    if(WIN32)
        set(ARCH_PATH "win64")
    else()
        set(ARCH_PATH "linux64")
    endif()
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    if(WIN32)
        set(ARCH_PATH "win32")
    else()
        set(ARCH_PATH "linux32")
    endif()
else()
    message(FATAL_ERROR "Package only supports x64 and x86 Windows and Linux.")
endif()

if(VCPKG_CMAKE_SYSTEM_NAME AND NOT (VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(FATAL_ERROR "Package only supports Windows or Linux desktop.")
endif()

file(MAKE_DIRECTORY
    ${CURRENT_PACKAGES_DIR}/lib
    ${CURRENT_PACKAGES_DIR}/bin
    ${CURRENT_PACKAGES_DIR}/debug/lib
    ${CURRENT_PACKAGES_DIR}/debug/bin
)

if(WIN32)
    file(COPY ${SOURCE_PATH}/lib/${ARCH_PATH}/openvr_api.lib DESTINATION ${CURRENT_PACKAGES_DIR}/lib)
    file(COPY ${SOURCE_PATH}/lib/${ARCH_PATH}/openvr_api.lib DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib)
    file(COPY
        ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.dll
        ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.pdb
        DESTINATION ${CURRENT_PACKAGES_DIR}/bin
    )
    file(COPY
        ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.dll
        ${SOURCE_PATH}/bin/${ARCH_PATH}/openvr_api.pdb
        DESTINATION ${CURRENT_PACKAGES_DIR}/debug/bin
    )
else()
    file(COPY ${SOURCE_PATH}/lib/${ARCH_PATH}/libopenvr_api.so DESTINATION ${CURRENT_PACKAGES_DIR}/lib)
    file(COPY ${SOURCE_PATH}/lib/${ARCH_PATH}/libopenvr_api.so DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib)
    file(COPY
        ${SOURCE_PATH}/bin/${ARCH_PATH}/libopenvr_api.so
        ${SOURCE_PATH}/bin/${ARCH_PATH}/libopenvr_api.so.dbg
        DESTINATION ${CURRENT_PACKAGES_DIR}/bin
    )
    file(COPY
        ${SOURCE_PATH}/bin/${ARCH_PATH}/libopenvr_api.so
        ${SOURCE_PATH}/bin/${ARCH_PATH}/libopenvr_api.so.dbg
        DESTINATION ${CURRENT_PACKAGES_DIR}/debug/bin
    )
endif()

file(COPY ${SOURCE_PATH}/headers DESTINATION ${CURRENT_PACKAGES_DIR})
file(RENAME ${CURRENT_PACKAGES_DIR}/headers ${CURRENT_PACKAGES_DIR}/include)

file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/openvr)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/openvr/LICENSE ${CURRENT_PACKAGES_DIR}/share/openvr/copyright)
