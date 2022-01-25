if(VCPKG_CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "This port is only for building openssl on Windows Desktop")
endif()

include(vcpkg_common_functions)
set(OPENSSL_VERSION 1.1.1h)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src/openssl-${OPENSSL_VERSION})

vcpkg_find_acquire_program(PERL)

get_filename_component(PERL_EXE_PATH ${PERL} DIRECTORY)
set(ENV{PATH} "$ENV{PATH};${PERL_EXE_PATH}")

vcpkg_download_distfile(OPENSSL_SOURCE_ARCHIVE
    URLS "https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz" "https://www.openssl.org/source/old/1.1.1/openssl-${OPENSSL_VERSION}.tar.gz"
    FILENAME "openssl-${OPENSSL_VERSION}.tar.gz"
    SHA512 da50fd99325841ed7a4367d9251c771ce505a443a73b327d8a46b2c6a7d2ea99e43551a164efc86f8743b22c2bdb0020bf24a9cbd445e9d68868b2dc1d34033a
)

vcpkg_extract_source_archive(${OPENSSL_SOURCE_ARCHIVE})

vcpkg_find_acquire_program(NASM)
get_filename_component(NASM_EXE_PATH ${NASM} DIRECTORY)
set(ENV{PATH} "${NASM_EXE_PATH};$ENV{PATH}")

vcpkg_find_acquire_program(JOM)

set(CONFIGURE_COMMAND ${PERL} Configure
    enable-static-engine
    enable-capieng
    no-ssl2
    -utf-8
)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(OPENSSL_ARCH VC-WIN32)
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(OPENSSL_ARCH VC-WIN64A)
else()
    message(FATAL_ERROR "Unsupported target architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

file(REMOVE_RECURSE ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg)


if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    message(STATUS "Configure ${TARGET_TRIPLET}-rel")
    file(COPY ${MASTER_COPY_SOURCE_PATH} DESTINATION ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel)
    set(SOURCE_PATH_RELEASE ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/openssl-${OPENSSL_VERSION})
    set(OPENSSLDIR_RELEASE ${CURRENT_PACKAGES_DIR})

    vcpkg_execute_required_process(
        COMMAND ${CONFIGURE_COMMAND} ${OPENSSL_ARCH} "--prefix=${OPENSSLDIR_RELEASE}" "--openssldir=${OPENSSLDIR_RELEASE}" -FS
        WORKING_DIRECTORY ${SOURCE_PATH_RELEASE}
        LOGNAME configure-perl-${TARGET_TRIPLET}-${CMAKE_BUILD_TYPE}-rel
    )
    message(STATUS "Configure ${TARGET_TRIPLET}-rel done")

    message(STATUS "Build ${TARGET_TRIPLET}-rel")
    # Openssl's buildsystem has a race condition which will cause JOM to fail at some point.
    # This is ok; we just do as much work as we can in parallel first, then follow up with a single-threaded build.
    make_directory(${SOURCE_PATH_RELEASE}/inc32/openssl)
    execute_process(
        COMMAND ${JOM} -k -j $ENV{NUMBER_OF_PROCESSORS}
        WORKING_DIRECTORY ${SOURCE_PATH_RELEASE}
        OUTPUT_FILE ${CURRENT_BUILDTREES_DIR}/build-${TARGET_TRIPLET}-rel-0-out.log
        ERROR_FILE ${CURRENT_BUILDTREES_DIR}/build-${TARGET_TRIPLET}-rel-0-err.log
    )
    vcpkg_execute_required_process(
        COMMAND nmake install
        WORKING_DIRECTORY ${SOURCE_PATH_RELEASE}
        LOGNAME build-${TARGET_TRIPLET}-rel-1
    )
    message(STATUS "Build ${TARGET_TRIPLET}-rel done")
endif()


if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    message(STATUS "Configure ${TARGET_TRIPLET}-dbg")
    file(COPY ${MASTER_COPY_SOURCE_PATH} DESTINATION ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg)
    set(SOURCE_PATH_DEBUG ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/openssl-${OPENSSL_VERSION})
    set(OPENSSLDIR_DEBUG ${CURRENT_PACKAGES_DIR}/debug)

    vcpkg_execute_required_process(
        COMMAND ${CONFIGURE_COMMAND} ${OPENSSL_ARCH} --debug "--prefix=${OPENSSLDIR_DEBUG}" "--openssldir=${OPENSSLDIR_DEBUG}" -FS
        WORKING_DIRECTORY ${SOURCE_PATH_DEBUG}
        LOGNAME configure-perl-${TARGET_TRIPLET}-${CMAKE_BUILD_TYPE}-dbg
    )
    message(STATUS "Configure ${TARGET_TRIPLET}-dbg done")

    message(STATUS "Build ${TARGET_TRIPLET}-dbg")
    make_directory(${SOURCE_PATH_DEBUG}/inc32/openssl)
    execute_process(
        COMMAND ${JOM} -k -j $ENV{NUMBER_OF_PROCESSORS}
        WORKING_DIRECTORY ${SOURCE_PATH_DEBUG}
        OUTPUT_FILE ${CURRENT_BUILDTREES_DIR}/build-${TARGET_TRIPLET}-dbg-0-out.log
        ERROR_FILE ${CURRENT_BUILDTREES_DIR}/build-${TARGET_TRIPLET}-dbg-0-err.log
    )
    vcpkg_execute_required_process(
        COMMAND nmake install
        WORKING_DIRECTORY ${SOURCE_PATH_DEBUG}
        LOGNAME build-${TARGET_TRIPLET}-dbg-1
    )
    message(STATUS "Build ${TARGET_TRIPLET}-dbg done")
endif()


file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/certs)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/engines-1_1)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/private)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/certs)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/engines-1_1)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/private)
file(REMOVE
    ${CURRENT_PACKAGES_DIR}/openssl.cnf
    ${CURRENT_PACKAGES_DIR}/openssl.cnf.dist
    ${CURRENT_PACKAGES_DIR}/ct_log_list.cnf
    ${CURRENT_PACKAGES_DIR}/ct_log_list.cnf.dist
    ${CURRENT_PACKAGES_DIR}/debug/openssl.cnf
    ${CURRENT_PACKAGES_DIR}/debug/openssl.cnf.dist
    ${CURRENT_PACKAGES_DIR}/debug/ct_log_list.cnf
    ${CURRENT_PACKAGES_DIR}/debug/ct_log_list.cnf.dist
    ${CURRENT_PACKAGES_DIR}/debug/bin/openssl.exe
)

file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/tools/openssl/)
file(RENAME ${CURRENT_PACKAGES_DIR}/bin/openssl.exe ${CURRENT_PACKAGES_DIR}/tools/openssl/openssl.exe)

vcpkg_copy_tool_dependencies(${CURRENT_PACKAGES_DIR}/tools/openssl)

if(VCPKG_LIBRARY_LINKAGE STREQUAL static)
    # They should be empty, only the exes deleted above were in these directories
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/bin/)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin/)
endif()

vcpkg_copy_pdbs()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})
file(INSTALL ${MASTER_COPY_SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

vcpkg_test_cmake(PACKAGE_NAME OpenSSL MODULE)
