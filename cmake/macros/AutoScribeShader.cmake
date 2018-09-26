#
#  AutoScribeShader.cmake
#
#  Created by Sam Gateau on 12/17/14.
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(AUTOSCRIBE_SHADER)
    unset(SHADER_INCLUDE_FILES)
    # Grab include files
    foreach(includeFile  ${ARGN})
        list(APPEND SHADER_INCLUDE_FILES ${includeFile})
    endforeach()

    foreach(SHADER_INCLUDE ${SHADER_INCLUDE_FILES})
        get_filename_component(INCLUDE_DIR ${SHADER_INCLUDE} PATH)
        list(APPEND SHADER_INCLUDES_PATHS ${INCLUDE_DIR})
    endforeach()

    list(REMOVE_DUPLICATES SHADER_INCLUDES_PATHS)
    #Extract the unique include shader paths
    set(INCLUDES ${HIFI_LIBRARIES_SHADER_INCLUDE_FILES})
    foreach(EXTRA_SHADER_INCLUDE ${INCLUDES})
        list(APPEND SHADER_INCLUDES_PATHS ${EXTRA_SHADER_INCLUDE})
    endforeach()

    list(REMOVE_DUPLICATES SHADER_INCLUDES_PATHS)
    #message(ready for includes ${SHADER_INCLUDES_PATHS})

    # make the scribe include arguments
    set(SCRIBE_INCLUDES)
    foreach(INCLUDE_PATH ${SHADER_INCLUDES_PATHS})
        set(SCRIBE_INCLUDES ${SCRIBE_INCLUDES} -I ${INCLUDE_PATH}/)
    endforeach()

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
    file(MAKE_DIRECTORY "${SHADERS_DIR}/${SHADER_LIB}")
    set(SHADER_TARGET "${SHADERS_DIR}/${SHADER_LIB}/${SHADER_NAME}.${SHADER_TYPE}")
    file(TO_CMAKE_PATH "${SHADER_TARGET}" COMPILED_SHADER)
    set(REFLECTED_SHADER "${COMPILED_SHADER}.json") 

    set(SCRIBE_ARGS -T ${SHADER_TYPE} -D GLPROFILE ${GLPROFILE} ${SCRIBE_INCLUDES} -o ${SHADER_TARGET} ${SHADER_FILE})

    # Generate the frag/vert file
    add_custom_command(
        OUTPUT ${SHADER_TARGET}
        COMMAND ${SCRIBE_COMMAND} ${SCRIBE_ARGS} 
        DEPENDS ${SHADER_FILE} ${SCRIBE_COMMAND} ${SHADER_INCLUDE_FILES})

    # Generate the json reflection
    # FIXME move to spirv-cross for this task after we have spirv compatible shaders
    add_custom_command(
        OUTPUT ${REFLECTED_SHADER}
        COMMAND ${SHREFLECT_COMMAND} ${COMPILED_SHADER}
        DEPENDS ${SHREFLECT_DEPENDENCY} ${COMPILED_SHADER})

    #output the generated file name
    source_group("Compiled/${SHADER_LIB}" FILES ${COMPILED_SHADER})
    set_property(SOURCE ${COMPILED_SHADER} PROPERTY SKIP_AUTOMOC ON)
    list(APPEND COMPILED_SHADERS ${COMPILED_SHADER})

    source_group("Reflected/${SHADER_LIB}" FILES ${REFLECTED_SHADER})
    list(APPEND REFLECTED_SHADERS ${REFLECTED_SHADER})

    string(CONCAT SHADER_QRC "${SHADER_QRC}" "<file alias=\"${SHADER_COUNT}\">${COMPILED_SHADER}</file>\n")
    string(CONCAT SHADER_QRC "${SHADER_QRC}" "<file alias=\"${SHADER_COUNT}_reflection\">${REFLECTED_SHADER}</file>\n")
    string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "${SHADER_NAME} = ${SHADER_COUNT},\n")

    MATH(EXPR SHADER_COUNT "${SHADER_COUNT}+1")
endmacro()

macro(AUTOSCRIBE_SHADER_LIB)
    if (NOT ("${TARGET_NAME}" STREQUAL "shaders"))
        message(FATAL_ERROR "AUTOSCRIBE_SHADER_LIB can only be used by the shaders library")
    endif()

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
    #file(RELATIVE_PATH RELATIVE_LIBRARY_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR} "${HIFI_LIBRARY_DIR}")
    #foreach(HIFI_LIBRARY ${ARGN})
    #list(APPEND HIFI_LIBRARIES_SHADER_INCLUDE_FILES ${HIFI_LIBRARY_DIR}/${HIFI_LIBRARY}/src)
    #endforeach()
    #endif()
endmacro()

macro(AUTOSCRIBE_SHADER_LIBS)
    set(SCRIBE_COMMAND scribe)
    set(SHREFLECT_COMMAND shreflect)
    set(SHREFLECT_DEPENDENCY shreflect)

    # Target dependant Custom rule on the SHADER_FILE
    if (ANDROID)
        set(GLPROFILE LINUX_GL)
        set(SCRIBE_COMMAND ${NATIVE_SCRIBE})
        set(SHREFLECT_COMMAND ${NATIVE_SHREFLECT})
        unset(SHREFLECT_DEPENDENCY)
    else() 
        if (APPLE)
            set(GLPROFILE MAC_GL)
        elseif(UNIX)
            set(GLPROFILE LINUX_GL)
        else()
            set(GLPROFILE PC_GL)
        endif()
    endif()

    # Start the shader IDs
    set(SHADER_COUNT 1)
    set(SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(SHADER_ENUMS "")
    file(MAKE_DIRECTORY ${SHADERS_DIR})

    #
    # Scribe generation & program defintiion
    # 
    foreach(SHADER_LIB ${ARGN})
        AUTOSCRIBE_SHADER_LIB(${SHADER_LIB})
    endforeach()

    # Generate the library files
    configure_file(
        ShaderEnums.cpp.in 
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.cpp)
    configure_file(
        ShaderEnums.h.in 
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.h)
    configure_file(
        shaders.qrc.in 
        ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)

    set(AUTOSCRIBE_SHADER_LIB_SRC "${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.h;${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.cpp")
    set(QT_RESOURCES_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)
    
    # Custom targets required to force generation of the shaders via scribe
    add_custom_target(scribe_shaders SOURCES ${ALL_SCRIBE_SHADERS})
    add_custom_target(compiled_shaders SOURCES ${COMPILED_SHADERS})
    add_custom_target(reflected_shaders SOURCES ${REFLECTED_SHADERS})
    set_target_properties(scribe_shaders PROPERTIES FOLDER "Shaders")
    set_target_properties(compiled_shaders PROPERTIES FOLDER "Shaders")
    set_target_properties(reflected_shaders PROPERTIES FOLDER "Shaders")
endmacro()
