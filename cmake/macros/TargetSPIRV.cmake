macro(TARGET_SPIRV)
    add_dependency_external_projects(spirv_cross)
    target_link_libraries(${TARGET_NAME} ${SPIRV_CROSS_LIBRARIES})
    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})
    
    # spirv-tools requires spirv-headers
    add_dependency_external_projects(spirv_headers)
    add_dependency_external_projects(spirv_tools)
    target_link_libraries(${TARGET_NAME} ${SPIRV_TOOLS_LIBRARIES})
    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${SPIRV_TOOLS_INCLUDE_DIRS})
    
    add_dependency_external_projects(glslang)
    target_link_libraries(${TARGET_NAME} ${GLSLANG_LIBRARIES})
    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${GLSLANG_INCLUDE_DIRS})
endmacro()
