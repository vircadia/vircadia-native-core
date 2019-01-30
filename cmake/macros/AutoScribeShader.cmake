#
#  AutoScribeShader.cmake
#
#  Created by Sam Gateau on 12/17/14.
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

# FIXME use the built tools 

macro(AUTOSCRIBE_APPEND_QRC)
    string(CONCAT SHADER_QRC "${SHADER_QRC}" "<file alias=\"${ARGV0}\">${ARGV1}</file>\n")
endmacro()

macro(AUTOSCRIBE_PLATFORM_SHADER)
    set(AUTOSCRIBE_PLATFORM_PATH "${ARGV0}")
    string(REGEX MATCH "([0-9]+(es)?)(/stereo)?" PLATFORM_PATH_REGEX ${AUTOSCRIBE_PLATFORM_PATH})
    set(AUTOSCRIBE_DIALECT "${CMAKE_MATCH_1}")
    if (CMAKE_MATCH_3)
        set(AUTOSCRIBE_VARIANT "stereo")
    else()
        set(AUTOSCRIBE_VARIANT "mono")
    endif()
    string(REGEX REPLACE "/" "\\\\" SOURCE_GROUP_PATH ${AUTOSCRIBE_PLATFORM_PATH})
    set(SOURCE_GROUP_PATH "${SHADER_LIB}\\${SOURCE_GROUP_PATH}")
    set(AUTOSCRIBE_DIALECT_HEADER "${AUTOSCRIBE_HEADER_DIR}/${AUTOSCRIBE_DIALECT}/header.glsl")
    set(AUTOSCRIBE_VARIANT_HEADER "${AUTOSCRIBE_HEADER_DIR}/${AUTOSCRIBE_VARIANT}.glsl")

    set(AUTOSCRIBE_OUTPUT_FILE "${SHADERS_DIR}/${SHADER_LIB}/${AUTOSCRIBE_PLATFORM_PATH}/${SHADER_NAME}.${SHADER_TYPE}")
    AUTOSCRIBE_APPEND_QRC("${SHADER_COUNT}/${AUTOSCRIBE_PLATFORM_PATH}/scribe" "${AUTOSCRIBE_OUTPUT_FILE}")
    source_group(${SOURCE_GROUP_PATH} FILES ${AUTOSCRIBE_OUTPUT_FILE})
    set_property(SOURCE ${AUTOSCRIBE_OUTPUT_FILE} PROPERTY SKIP_AUTOMOC ON)
    list(APPEND SCRIBED_SHADERS ${AUTOSCRIBE_OUTPUT_FILE})

    set(AUTOSCRIBE_SPIRV_FILE "${AUTOSCRIBE_OUTPUT_FILE}.spv")
    # don't add unoptimized spirv to the QRC
    #AUTOSCRIBE_APPEND_QRC("${SHADER_COUNT}/${AUTOSCRIBE_PLATFORM_PATH}/spirv_unopt" "${AUTOSCRIBE_SPIRV_FILE}")
    source_group(${SOURCE_GROUP_PATH} FILES ${AUTOSCRIBE_SPIRV_FILE})
    set_property(SOURCE ${AUTOSCRIBE_SPIRV_FILE} PROPERTY SKIP_AUTOMOC ON)
    list(APPEND SPIRV_SHADERS ${AUTOSCRIBE_SPIRV_FILE})

    set(AUTOSCRIBE_SPIRV_OPT_FILE "${AUTOSCRIBE_OUTPUT_FILE}.opt.spv")
    AUTOSCRIBE_APPEND_QRC("${SHADER_COUNT}/${AUTOSCRIBE_PLATFORM_PATH}/spirv" "${AUTOSCRIBE_SPIRV_OPT_FILE}")
    source_group(${SOURCE_GROUP_PATH} FILES ${AUTOSCRIBE_SPIRV_OPT_FILE})
    set_property(SOURCE ${AUTOSCRIBE_SPIRV_OPT_FILE} PROPERTY SKIP_AUTOMOC ON)
    list(APPEND SPIRV_SHADERS ${AUTOSCRIBE_SPIRV_OPT_FILE})

    set(AUTOSCRIBE_SPIRV_GLSL_FILE "${AUTOSCRIBE_OUTPUT_FILE}.glsl")
    AUTOSCRIBE_APPEND_QRC("${SHADER_COUNT}/${AUTOSCRIBE_PLATFORM_PATH}/glsl" "${AUTOSCRIBE_SPIRV_GLSL_FILE}")
    source_group(${SOURCE_GROUP_PATH} FILES ${AUTOSCRIBE_SPIRV_GLSL_FILE})
    set_property(SOURCE ${AUTOSCRIBE_SPIRV_GLSL_FILE} PROPERTY SKIP_AUTOMOC ON)
    list(APPEND SPIRV_SHADERS ${AUTOSCRIBE_SPIRV_GLSL_FILE})

    set(AUTOSCRIBE_SPIRV_JSON_FILE "${AUTOSCRIBE_OUTPUT_FILE}.json")
    AUTOSCRIBE_APPEND_QRC("${SHADER_COUNT}/${AUTOSCRIBE_PLATFORM_PATH}/json" "${AUTOSCRIBE_SPIRV_JSON_FILE}")
    source_group(${SOURCE_GROUP_PATH} FILES ${AUTOSCRIBE_SPIRV_JSON_FILE})
    set_property(SOURCE ${AUTOSCRIBE_SPIRV_JSON_FILE} PROPERTY SKIP_AUTOMOC ON)
    list(APPEND REFLECTED_SHADERS ${AUTOSCRIBE_SPIRV_JSON_FILE})

    unset(SHADER_GEN_LINE)
    list(APPEND SHADER_GEN_LINE ${AUTOSCRIBE_DIALECT})
    list(APPEND SHADER_GEN_LINE ${AUTOSCRIBE_VARIANT})
    file(RELATIVE_PATH TEMP_PATH ${CMAKE_SOURCE_DIR} ${SHADER_FILE})
    list(APPEND SHADER_GEN_LINE ${TEMP_PATH})
    file(RELATIVE_PATH TEMP_PATH ${CMAKE_SOURCE_DIR} ${AUTOSCRIBE_OUTPUT_FILE})
    list(APPEND SHADER_GEN_LINE ${TEMP_PATH})
    list(APPEND SHADER_GEN_LINE ${AUTOSCRIBE_SHADER_SEEN_LIBS})
    string(CONCAT AUTOSCRIBE_SHADERGEN_COMMANDS "${AUTOSCRIBE_SHADERGEN_COMMANDS}" "${SHADER_GEN_LINE}\n")
endmacro()

macro(AUTOSCRIBE_SHADER)
    #
    # Set the include paths
    #
    # FIXME base the include paths off of output from the scribe tool,
    # instead of treating every previously seen shader as a possible header
    unset(SHADER_INCLUDE_FILES)
    foreach(includeFile  ${ARGN})
        list(APPEND SHADER_INCLUDE_FILES ${includeFile})
    endforeach()
    foreach(SHADER_INCLUDE ${SHADER_INCLUDE_FILES})
        get_filename_component(INCLUDE_DIR ${SHADER_INCLUDE} PATH)
        list(APPEND SHADER_INCLUDES_PATHS ${INCLUDE_DIR})
    endforeach()
    set(INCLUDES ${HIFI_LIBRARIES_SHADER_INCLUDE_FILES})
    foreach(EXTRA_SHADER_INCLUDE ${INCLUDES})
        list(APPEND SHADER_INCLUDES_PATHS ${EXTRA_SHADER_INCLUDE})
    endforeach()
    list(REMOVE_DUPLICATES SHADER_INCLUDES_PATHS)
    unset(SCRIBE_INCLUDES)
    foreach(INCLUDE_PATH ${SHADER_INCLUDES_PATHS})
        set(SCRIBE_INCLUDES ${SCRIBE_INCLUDES} -I ${INCLUDE_PATH}/)
    endforeach()

    #
    # Figure out the various output names
    #
    # Define the final name of the generated shader file
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
    if(SHADER_EXT STREQUAL .slv)
        set(SHADER_TYPE vert)
    elseif(${SHADER_EXT} STREQUAL .slf)
        set(SHADER_TYPE frag)
    elseif(${SHADER_EXT} STREQUAL .slg)
        set(SHADER_TYPE geom)
    endif()

    set(SCRIBE_ARGS -D GLPROFILE ${GLPROFILE} -T ${SHADER_TYPE} ${SCRIBE_INCLUDES} )

    # SHADER_SCRIBED -> the output of scribe
    set(SHADER_SCRIBED "${SHADERS_DIR}/${SHADER_LIB}/${SHADER_NAME}.${SHADER_TYPE}")

    # SHADER_NAME_FILE -> a file containing the shader name and extension (useful for debugging and for 
    # determining the type of shader from the filename)
    set(SHADER_NAME_FILE "${SHADER_SCRIBED}.name")
    file(TO_CMAKE_PATH "${SHADER_SCRIBED}" SHADER_SCRIBED)
    file(WRITE "${SHADER_SCRIBED}.name" "${SHADER_NAME}.${SHADER_TYPE}")
    AUTOSCRIBE_APPEND_QRC("${SHADER_COUNT}/name" "${SHADER_NAME_FILE}")

    if (USE_GLES)
        set(SPIRV_CROSS_ARGS --version 310es)
        AUTOSCRIBE_PLATFORM_SHADER("310es")
        AUTOSCRIBE_PLATFORM_SHADER("310es/stereo")
    else()
        set(SPIRV_CROSS_ARGS --version 410 --no-420pack-extension)
        AUTOSCRIBE_PLATFORM_SHADER("410")
        AUTOSCRIBE_PLATFORM_SHADER("410/stereo")
        if (NOT APPLE)
            set(SPIRV_CROSS_ARGS --version 450)
            AUTOSCRIBE_PLATFORM_SHADER("450")
            AUTOSCRIBE_PLATFORM_SHADER("450/stereo")
        endif()
    endif()

    string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "${SHADER_NAME} = ${SHADER_COUNT},\n")
    string(CONCAT SHADER_SHADERS_ARRAY  "${SHADER_SHADERS_ARRAY}" "${SHADER_COUNT},\n")
    MATH(EXPR SHADER_COUNT "${SHADER_COUNT}+1")
endmacro()

macro(AUTOSCRIBE_SHADER_LIB)
    if (NOT ("${TARGET_NAME}" STREQUAL "shaders"))
        message(FATAL_ERROR "AUTOSCRIBE_SHADER_LIB can only be used by the shaders library")
    endif()

    file(MAKE_DIRECTORY "${SHADERS_DIR}/${SHADER_LIB}")

    list(APPEND HIFI_LIBRARIES_SHADER_INCLUDE_FILES "${CMAKE_SOURCE_DIR}/libraries/${SHADER_LIB}/src") 
    string(REGEX REPLACE "[-]" "_" SHADER_NAMESPACE ${SHADER_LIB})
    string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace ${SHADER_NAMESPACE} {\n")
    set(SRC_FOLDER "${CMAKE_SOURCE_DIR}/libraries/${ARGN}/src")

    # Process the scribe headers
    file(GLOB_RECURSE SHADER_INCLUDE_FILES ${SRC_FOLDER}/*.slh)
    if(SHADER_INCLUDE_FILES)
        source_group("${SHADER_LIB}/Headers" FILES ${SHADER_INCLUDE_FILES})
        list(APPEND ALL_SHADER_HEADERS ${SHADER_INCLUDE_FILES})
        list(APPEND ALL_SCRIBE_SHADERS ${SHADER_INCLUDE_FILES})
    endif()

    file(GLOB_RECURSE SHADER_VERTEX_FILES ${SRC_FOLDER}/*.slv)
    if (SHADER_VERTEX_FILES)
        source_group("${SHADER_LIB}/Vertex" FILES ${SHADER_VERTEX_FILES})
        list(APPEND ALL_SCRIBE_SHADERS ${SHADER_VERTEX_FILES})
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace vertex { enum {\n")
        foreach(SHADER_FILE ${SHADER_VERTEX_FILES}) 
            AUTOSCRIBE_SHADER(${ALL_SHADER_HEADERS})
        endforeach()
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "}; } // vertex \n")
    endif()

    file(GLOB_RECURSE SHADER_FRAGMENT_FILES ${SRC_FOLDER}/*.slf)
    if (SHADER_FRAGMENT_FILES)
        source_group("${SHADER_LIB}/Fragment" FILES ${SHADER_FRAGMENT_FILES})
        list(APPEND ALL_SCRIBE_SHADERS ${SHADER_FRAGMENT_FILES})
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace fragment { enum {\n")
        foreach(SHADER_FILE ${SHADER_FRAGMENT_FILES}) 
            AUTOSCRIBE_SHADER(${ALL_SHADER_HEADERS})
        endforeach()
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "}; } // fragment \n")
    endif()
    
    # FIXME add support for geometry, compute and tesselation shaders
    #file(GLOB_RECURSE SHADER_GEOMETRY_FILES ${SRC_FOLDER}/*.slg)
    #file(GLOB_RECURSE SHADER_COMPUTE_FILES ${SRC_FOLDER}/*.slc)

    # the programs
    file(GLOB_RECURSE SHADER_PROGRAM_FILES ${SRC_FOLDER}/*.slp)
    if (SHADER_PROGRAM_FILES)
        source_group("${SHADER_LIB}/Program" FILES ${SHADER_PROGRAM_FILES})
        list(APPEND ALL_SCRIBE_SHADERS ${SHADER_PROGRAM_FILES})
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace program { enum {\n")
        foreach(PROGRAM_FILE ${SHADER_PROGRAM_FILES})
            get_filename_component(PROGRAM_NAME ${PROGRAM_FILE} NAME_WE)
            set(AUTOSCRIBE_PROGRAM_FRAGMENT ${PROGRAM_NAME})
            file(READ ${PROGRAM_FILE} PROGRAM_CONFIG)
            set(AUTOSCRIBE_PROGRAM_VERTEX ${PROGRAM_NAME})
            set(AUTOSCRIBE_PROGRAM_FRAGMENT ${PROGRAM_NAME})

            if (NOT("${PROGRAM_CONFIG}" STREQUAL ""))
                string(REGEX MATCH ".*VERTEX +([_\\:A-Z0-9a-z]+)" MVERT ${PROGRAM_CONFIG})
                if (CMAKE_MATCH_1)
                    set(AUTOSCRIBE_PROGRAM_VERTEX ${CMAKE_MATCH_1})
                endif()
                string(REGEX MATCH ".*FRAGMENT +([_:A-Z0-9a-z]+)" MFRAG ${PROGRAM_CONFIG})
                if (CMAKE_MATCH_1)
                    set(AUTOSCRIBE_PROGRAM_FRAGMENT ${CMAKE_MATCH_1})
                endif()
            endif()

            if (NOT (${AUTOSCRIBE_PROGRAM_VERTEX} MATCHES ".*::.*"))
                set(AUTOSCRIBE_PROGRAM_VERTEX "vertex::${AUTOSCRIBE_PROGRAM_VERTEX}")
            endif()
            if (NOT (${AUTOSCRIBE_PROGRAM_FRAGMENT} MATCHES ".*::.*"))
                set(AUTOSCRIBE_PROGRAM_FRAGMENT "fragment::${AUTOSCRIBE_PROGRAM_FRAGMENT}")
            endif()

            set(PROGRAM_ENTRY "${PROGRAM_NAME} = (${AUTOSCRIBE_PROGRAM_VERTEX} << 16) |  ${AUTOSCRIBE_PROGRAM_FRAGMENT},\n")
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "${PROGRAM_ENTRY}")
            string(CONCAT SHADER_PROGRAMS_ARRAY "${SHADER_PROGRAMS_ARRAY} ${SHADER_NAMESPACE}::program::${PROGRAM_NAME},\n")
        endforeach()
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "}; } // program \n")
    endif()

    # Finish the shader enums 
    string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "} // namespace ${SHADER_NAMESPACE}\n")
endmacro()

macro(AUTOSCRIBE_SHADER_LIBS)
    message(STATUS "Shader processing start")
    set(AUTOSCRIBE_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/headers)
    # Start the shader IDs
    set(SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY ${SHADERS_DIR})
    set(SHADER_ENUMS "")
    set(SHADER_COUNT 1)

    #
    # Scribe generation & program defintiion
    # 
    foreach(SHADER_LIB ${ARGN})
        list(APPEND AUTOSCRIBE_SHADER_SEEN_LIBS ${SHADER_LIB})
        AUTOSCRIBE_SHADER_LIB(${SHADER_LIB})
    endforeach()

    # Generate the library files
    configure_file(
        ShaderEnums.cpp.in 
        ${CMAKE_CURRENT_BINARY_DIR}/ShaderEnums.cpp)
    configure_file(
        ShaderEnums.h.in 
        ${CMAKE_CURRENT_BINARY_DIR}/ShaderEnums.h)

    configure_file(shaders.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)
    list(APPEND QT_RESOURCES_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)

    list(APPEND AUTOSCRIBE_SHADER_HEADERS ${AUTOSCRIBE_HEADER_DIR}/mono.glsl ${AUTOSCRIBE_HEADER_DIR}/stereo.glsl)
    list(APPEND AUTOSCRIBE_SHADER_HEADERS ${AUTOSCRIBE_HEADER_DIR}/450/header.glsl ${AUTOSCRIBE_HEADER_DIR}/410/header.glsl ${AUTOSCRIBE_HEADER_DIR}/310es/header.glsl)
    source_group("Shader Headers" FILES ${AUTOSCRIBE_HEADER_DIR}/mono.glsl ${AUTOSCRIBE_HEADER_DIR}/stereo.glsl)
    source_group("Shader Headers\\450" FILES ${AUTOSCRIBE_HEADER_DIR}/450/header.glsl)
    source_group("Shader Headers\\410" FILES ${AUTOSCRIBE_HEADER_DIR}/410/header.glsl)
    source_group("Shader Headers\\310es" FILES ${AUTOSCRIBE_HEADER_DIR}/310es/header.glsl)

    list(APPEND AUTOSCRIBE_SHADER_LIB_SRC ${AUTOSCRIBE_SHADER_HEADERS})
    list(APPEND AUTOSCRIBE_SHADER_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/ShaderEnums.h ${CMAKE_CURRENT_BINARY_DIR}/ShaderEnums.cpp)
    
    # Write the shadergen command list
    set(AUTOSCRIBE_SHADERGEN_COMMANDS_FILE ${CMAKE_CURRENT_BINARY_DIR}/shadergen.txt)
    file(WRITE ${AUTOSCRIBE_SHADERGEN_COMMANDS_FILE} "${AUTOSCRIBE_SHADERGEN_COMMANDS}")

    # A custom python script which will generate all our shader artifacts
    add_custom_command(
        OUTPUT ${SCRIBED_SHADERS} ${SPIRV_SHADERS} ${REFLECTED_SHADERS}
        COMMENT "Generating/updating shaders"
        COMMAND ${HIFI_PYTHON_EXEC} ${CMAKE_SOURCE_DIR}/tools/shadergen.py 
            --commands ${AUTOSCRIBE_SHADERGEN_COMMANDS_FILE} 
            --tools-dir ${VCPKG_TOOLS_DIR}
            --build-dir ${CMAKE_CURRENT_BINARY_DIR}
            --source-dir ${CMAKE_SOURCE_DIR}
        DEPENDS ${AUTOSCRIBE_SHADER_HEADERS} ${CMAKE_SOURCE_DIR}/tools/shadergen.py ${ALL_SCRIBE_SHADERS})

    add_custom_target(shadergen DEPENDS ${SCRIBED_SHADERS} ${SPIRV_SHADERS} ${REFLECTED_SHADERS})
    set_target_properties(shadergen PROPERTIES FOLDER "Shaders")

    # Custom targets required to force generation of the shaders via scribe
    add_custom_target(scribe_shaders SOURCES ${ALL_SCRIBE_SHADERS} ${AUTOSCRIBE_SHADER_HEADERS})
    set_target_properties(scribe_shaders PROPERTIES FOLDER "Shaders")

    add_custom_target(scribed_shaders SOURCES ${SCRIBED_SHADERS})
    set_target_properties(scribed_shaders PROPERTIES FOLDER "Shaders")
    add_dependencies(scribed_shaders shadergen)

    add_custom_target(spirv_shaders SOURCES ${SPIRV_SHADERS})
    set_target_properties(spirv_shaders PROPERTIES FOLDER "Shaders")
    add_dependencies(spirv_shaders shadergen)

    add_custom_target(reflected_shaders SOURCES ${REFLECTED_SHADERS})
    set_target_properties(reflected_shaders PROPERTIES FOLDER "Shaders")
    add_dependencies(reflected_shaders shadergen)

    message(STATUS "Shader processing end")
endmacro()


