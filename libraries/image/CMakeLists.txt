set(TARGET_NAME image)
setup_hifi_library()
link_hifi_libraries(shared gpu)

target_glm()

add_dependency_external_projects(nvtt)
find_package(NVTT REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${NVTT_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${NVTT_LIBRARIES})
add_paths_to_fixup_libs(${NVTT_DLL_PATH})
