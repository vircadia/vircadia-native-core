include(vcpkg_common_functions)

set(SOURCE_VERSION 3.3.0)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    REF 3a7249f313b047417fbb1d36a3fbe6c3bf1505b5
    SHA512 ddeac01a25bfe47d4e749d150aeaea43760c9732db78bf6d15231b9de1b7fb5218f1ced07710cb1929175ddcb05848239edaf0f6cc47be48896211293ffd236c
    HEAD_REF master
)

file(INSTALL ${SOURCE_PATH}/src/vk_mem_alloc.h DESTINATION ${CURRENT_PACKAGES_DIR}/include/vma)
file(INSTALL ${SOURCE_PATH}/LICENSE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/vulkanmemoryallocator RENAME copyright)
