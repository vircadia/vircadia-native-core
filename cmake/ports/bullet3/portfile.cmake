# Updated June 6th, 2019, to force new vckpg hash
#
# Common Ambient Variables:
#
#   CURRENT_BUILDTREES_DIR    = ${VCPKG_ROOT_DIR}\buildtrees\${PORT}
#   CURRENT_PACKAGES_DIR      = ${VCPKG_ROOT_DIR}\packages\${PORT}_${TARGET_TRIPLET}
#   CURRENT_PORT_DIR          = ${VCPKG_ROOT_DIR}\ports\${PORT}
#   PORT                      = current port name (zlib, etc)
#   TARGET_TRIPLET            = current triplet (x86-windows, x64-windows-static, etc)
#   VCPKG_CRT_LINKAGE         = C runtime linkage type (static, dynamic)
#   VCPKG_LIBRARY_LINKAGE     = target library linkage type (static, dynamic)
#   VCPKG_ROOT_DIR            = <C:\path\to\current\vcpkg>
#   VCPKG_TARGET_ARCHITECTURE = target architecture (x64, x86, arm)
#

include(vcpkg_common_functions)


if (VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
    message(WARNING "Dynamic not supported, building static")
    set(VCPKG_LIBRARY_LINKAGE static)
    set(VCPKG_CRT_LINKAGE dynamic)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO bulletphysics/bullet3
    REF ab8f16961e19a86ee20c6a1d61f662392524cc77
    SHA512 927742db29867517283d45e475f0c534a9a57e165cae221f26e08e88057253a1682ac9919b2dc547b9cf388ba0b931b175623461d44f28c9184796ba90b1ed55
    HEAD_REF master
    PATCHES "bullet-git-fix-build-clang-8.patch"
)

if(WIN32)
    set(VIRCADIA_BULLET_OPTIONS "")
else()
    if(EXISTS "${VCPKG_ROOT_DIR}/_env/VIRCADIA_OPTIMIZE.txt")
        file(READ "${VCPKG_ROOT_DIR}/_env/VIRCADIA_OPTIMIZE.txt" VIRCADIA_OPTIMIZE)
    endif()
    if(EXISTS "${VCPKG_ROOT_DIR}/_env/VIRCADIA_CPU_ARCHITECTURE.txt")
        file(READ "${VCPKG_ROOT_DIR}/_env/VIRCADIA_CPU_ARCHITECTURE.txt" VIRCADIA_CPU_ARCHITECTURE)
    endif()


    if(VIRCADIA_OPTIMIZE)
        set(VIRCADIA_BULLET_OPTIONS "-DCMAKE_BUILD_TYPE=Release")
    else()
        set(VIRCADIA_BULLET_OPTIONS "-DCMAKE_BUILD_TYPE=RelWithDebInfo")
    endif()

    set(VIRCADIA_BULLET_OPTIONS "${VIRCADIA_BULLET_OPTIONS}")

    if(DEFINED VIRCADIA_CPU_ARCHITECTURE)
        set(VIRCADIA_BULLET_OPTIONS "${VIRCADIA_BULLET_OPTIONS} -DCMAKE_CXX_FLAGS=\"${VIRCADIA_CPU_ARCHITECTURE}\" -DCMAKE_C_FLAGS=\"${VIRCADIA_CPU_ARCHITECTURE}\" ")
    endif()
endif()

message("Optimization options for Bullet: ${VIRCADIA_BULLET_OPTIONS}")

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON
        -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON
        -DUSE_GLUT=0
        -DUSE_DX11=0
        -DBUILD_DEMOS=OFF
        -DBUILD_OPENGL3_DEMOS=OFF
        -DBUILD_BULLET3=OFF
        -DBUILD_BULLET2_DEMOS=OFF
        -DBUILD_CPU_DEMOS=OFF
        -DBUILD_EXTRAS=OFF
        -DBUILD_UNIT_TESTS=OFF
        -DBUILD_SHARED_LIBS=ON
        -DINSTALL_LIBS=ON
        ${VIRCADIA_BULLET_OPTIONS}
)

vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/cmake)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/cmake)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/include/bullet/BulletInverseDynamics/details)

vcpkg_copy_pdbs()

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/bullet3 RENAME copyright)
