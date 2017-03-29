set(TARGET_NAME model)
AUTOSCRIBE_SHADER_LIB(gpu model)
setup_hifi_library()
link_hifi_libraries(shared ktx gpu)

add_dependency_external_projects(nvtt)
target_include_directories(${TARGET_NAME} PRIVATE ${NVTT_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${NVTT_LIBRARIES})
