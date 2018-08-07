#
#  AutoScribeShader.cmake
#
#  Created by Sam Gateau on 12/17/14.
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

function(global_append varName varValue) 
    get_property(LOCAL_LIST GLOBAL PROPERTY ${varName})
    list(APPEND LOCAL_LIST ${varValue})
    set_property(GLOBAL PROPERTY ${varName} ${LOCAL_LIST})
endfunction()

function(AUTOSCRIBE_SHADER SHADER_FILE)
  # Grab include files
  foreach(includeFile  ${ARGN})
    list(APPEND SHADER_INCLUDE_FILES ${includeFile})
  endforeach()

  foreach(SHADER_INCLUDE ${SHADER_INCLUDE_FILES})
    get_filename_component(INCLUDE_DIR ${SHADER_INCLUDE} PATH)
    list(APPEND SHADER_INCLUDES_PATHS ${INCLUDE_DIR})
  endforeach()

  #Extract the unique include shader paths
  set(INCLUDES ${HIFI_LIBRARIES_SHADER_INCLUDE_FILES})
  #message(${TARGET_NAME} Hifi for includes ${INCLUDES})
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
  get_filename_component(SHADER_TARGET ${SHADER_FILE} NAME_WE)
  get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
  if(SHADER_EXT STREQUAL .slv)
    set(SHADER_TYPE vert)
  elseif(${SHADER_EXT} STREQUAL .slf)
    set(SHADER_TYPE frag)
  elseif(${SHADER_EXT} STREQUAL .slg)
    set(SHADER_TYPE geom)
  endif()
  file(MAKE_DIRECTORY "${SHADERS_DIR}/${SHADER_LIB}")
  set(SHADER_TARGET "${SHADERS_DIR}/${SHADER_LIB}/${SHADER_TARGET}.${SHADER_TYPE}")
  set(SCRIBE_COMMAND scribe)

  # Target dependant Custom rule on the SHADER_FILE
  if (APPLE)
    set(GLPROFILE MAC_GL)
  elseif (ANDROID)
    set(GLPROFILE LINUX_GL)
    set(SCRIBE_COMMAND ${NATIVE_SCRIBE})
  elseif (UNIX)
    set(GLPROFILE LINUX_GL)
  else ()
    set(GLPROFILE PC_GL)
  endif()
  set(SCRIBE_ARGS -T ${SHADER_TYPE} -D GLPROFILE ${GLPROFILE} ${SCRIBE_INCLUDES} -o ${SHADER_TARGET} ${SHADER_FILE})
  add_custom_command(
    OUTPUT ${SHADER_TARGET}
    COMMAND ${SCRIBE_COMMAND} ${SCRIBE_ARGS} 
    DEPENDS ${SHADER_FILE} ${SCRIBE_COMMAND} ${SHADER_INCLUDE_FILES}
  )

  #output the generated file name
  set(AUTOSCRIBE_SHADER_RETURN ${SHADER_TARGET} PARENT_SCOPE)
endfunction()

macro(AUTOSCRIBE_APPEND_SHADER_SOURCES)
    if (NOT("${ARGN}" STREQUAL "")) 
        set_property(GLOBAL PROPERTY ${TARGET_NAME}_SHADER_SOURCES "${ARGN}")
        global_append(GLOBAL_SHADER_LIBS ${TARGET_NAME})
        global_append(GLOBAL_SHADER_SOURCES "${ARGN}")
    endif()
endmacro()

macro(AUTOSCRIBE_SHADER_LIB)
  unset(HIFI_LIBRARIES_SHADER_INCLUDE_FILES)
  set(SRC_FOLDER "${CMAKE_SOURCE_DIR}/libraries/${TARGET_NAME}/src")
 
  file(GLOB_RECURSE SHADER_INCLUDE_FILES ${SRC_FOLDER}/*.slh)
  file(GLOB_RECURSE SHADER_VERTEX_FILES ${SRC_FOLDER}/*.slv)
  file(GLOB_RECURSE SHADER_FRAGMENT_FILES ${SRC_FOLDER}/*.slf)
  file(GLOB_RECURSE SHADER_GEOMETRY_FILES ${SRC_FOLDER}/*.slg)
  file(GLOB_RECURSE SHADER_COMPUTE_FILES ${SRC_FOLDER}/*.slc)
  
  unset(SHADER_SOURCE_FILES)
  list(APPEND SHADER_SOURCE_FILES ${SHADER_VERTEX_FILES})
  list(APPEND SHADER_SOURCE_FILES ${SHADER_FRAGMENT_FILES})
  list(APPEND SHADER_SOURCE_FILES ${SHADER_GEOMETRY_FILES})
  list(APPEND SHADER_SOURCE_FILES ${SHADER_COMPUTE_FILES})
  
  unset(LOCAL_SHADER_SOURCES)
  list(APPEND LOCAL_SHADER_SOURCES ${SHADER_SOURCE_FILES})
  list(APPEND LOCAL_SHADER_SOURCES ${SHADER_INCLUDE_FILES})

  AUTOSCRIBE_APPEND_SHADER_SOURCES(${LOCAL_SHADER_SOURCES})
  
  file(RELATIVE_PATH RELATIVE_LIBRARY_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR} "${HIFI_LIBRARY_DIR}")
  foreach(HIFI_LIBRARY ${ARGN})
    list(APPEND HIFI_LIBRARIES_SHADER_INCLUDE_FILES ${HIFI_LIBRARY_DIR}/${HIFI_LIBRARY}/src)
  endforeach()
endmacro()

macro(AUTOSCRIBE_PROGRAM)
    set(oneValueArgs NAME VERTEX FRAGMENT GEOMETRY COMPUTE)
    cmake_parse_arguments(AUTOSCRIBE_PROGRAM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT (DEFINED AUTOSCRIBE_PROGRAM_NAME))
        message(FATAL_ERROR "Programs must have a name and both a vertex and fragment shader")
    endif()
    if (NOT (DEFINED AUTOSCRIBE_PROGRAM_VERTEX))
        set(AUTOSCRIBE_PROGRAM_VERTEX ${AUTOSCRIBE_PROGRAM_NAME})
    endif()
    if (NOT (DEFINED AUTOSCRIBE_PROGRAM_FRAGMENT))
        set(AUTOSCRIBE_PROGRAM_FRAGMENT ${AUTOSCRIBE_PROGRAM_NAME})
    endif()

    if (NOT (${AUTOSCRIBE_PROGRAM_VERTEX} MATCHES ".*::.*"))
        set(AUTOSCRIBE_PROGRAM_VERTEX "vertex::${AUTOSCRIBE_PROGRAM_VERTEX}")
    endif()
    if (NOT (${AUTOSCRIBE_PROGRAM_FRAGMENT} MATCHES ".*::.*"))
        set(AUTOSCRIBE_PROGRAM_FRAGMENT "fragment::${AUTOSCRIBE_PROGRAM_FRAGMENT}")
    endif()

    unset(AUTOSCRIBE_PROGRAM_MAP)
    list(APPEND AUTOSCRIBE_PROGRAM_MAP AUTOSCRIBE_PROGRAM_VERTEX)
    list(APPEND AUTOSCRIBE_PROGRAM_MAP ${AUTOSCRIBE_PROGRAM_VERTEX})
    list(APPEND AUTOSCRIBE_PROGRAM_MAP AUTOSCRIBE_PROGRAM_FRAGMENT)
    list(APPEND AUTOSCRIBE_PROGRAM_MAP ${AUTOSCRIBE_PROGRAM_FRAGMENT})
    global_append(${TARGET_NAME}_PROGRAMS ${AUTOSCRIBE_PROGRAM_NAME})
    set_property(GLOBAL PROPERTY ${AUTOSCRIBE_PROGRAM_NAME} "${AUTOSCRIBE_PROGRAM_MAP}")
endmacro()

macro(unpack_map)
    set(MAP_VAR "${ARGN}")
    list(LENGTH MAP_VAR MAP_SIZE)
    MATH(EXPR MAP_ENTRY_RANGE "(${MAP_SIZE} / 2) - 1")
    foreach(INDEX RANGE ${MAP_ENTRY_RANGE})
        MATH(EXPR INDEX_NAME "${INDEX} * 2")
        MATH(EXPR INDEX_VAL "${INDEX_NAME} + 1")
        list(GET MAP_VAR ${INDEX_NAME} VAR_NAME)
        list(GET MAP_VAR ${INDEX_VAL} VAR_VAL)
        set(${VAR_NAME} ${VAR_VAL})
    endforeach()
endmacro()

macro(PROCESS_SHADER_FILE)
    AUTOSCRIBE_SHADER(${SHADER} ${ALL_SHADER_HEADERS} ${HIFI_LIBRARIES_SHADER_INCLUDE_FILES})
    file(TO_CMAKE_PATH "${AUTOSCRIBE_SHADER_RETURN}" AUTOSCRIBE_GENERATED_FILE)
    set_property(SOURCE ${AUTOSCRIBE_GENERATED_FILE} PROPERTY SKIP_AUTOMOC ON)
    source_group("Compiled/${SHADER_LIB}" FILES ${AUTOSCRIBE_GENERATED_FILE})
    set(REFLECTED_SHADER "${AUTOSCRIBE_GENERATED_FILE}.json")
    source_group("Reflected/${SHADER_LIB}" FILES ${REFLECTED_SHADER})
    list(APPEND COMPILED_SHADERS ${AUTOSCRIBE_GENERATED_FILE})
    get_filename_component(ENUM_NAME ${SHADER} NAME_WE)
    string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "${ENUM_NAME} = ${SHADER_COUNT},\n")
    MATH(EXPR SHADER_COUNT "${SHADER_COUNT}+1")
endmacro()

macro(AUTOSCRIBE_SHADER_FINISH)
    get_property(GLOBAL_SHADER_LIBS GLOBAL PROPERTY GLOBAL_SHADER_LIBS)
    list(REMOVE_DUPLICATES GLOBAL_SHADER_LIBS)
    set(SHADER_COUNT 0)
    set(PROGRAM_COUNT 0)
    set(SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(SHADER_ENUMS "")
    file(MAKE_DIRECTORY ${SHADERS_DIR})

    unset(COMPILED_SHADERS)
    foreach(SHADER_LIB ${GLOBAL_SHADER_LIBS})
        get_property(LIB_SHADER_SOURCES GLOBAL PROPERTY ${SHADER_LIB}_SHADER_SOURCES)
        get_property(LIB_PROGRAMS GLOBAL PROPERTY ${SHADER_LIB}_PROGRAMS)
        list(REMOVE_DUPLICATES LIB_SHADER_SOURCES)
        string(REGEX REPLACE "[-]" "_" SHADER_NAMESPACE ${SHADER_LIB})
        list(APPEND HIFI_LIBRARIES_SHADER_INCLUDE_FILES "${CMAKE_SOURCE_DIR}/libraries/${SHADER_LIB}/src") 

        unset(VERTEX_SHADERS)
        unset(FRAGMENT_SHADERS)
        unset(SHADER_HEADERS)

        foreach(SHADER_FILE ${LIB_SHADER_SOURCES})
            if (SHADER_FILE MATCHES ".*\\.slv")
                list(APPEND VERTEX_SHADERS ${SHADER_FILE})
            elseif (SHADER_FILE MATCHES ".*\\.slf")
                list(APPEND FRAGMENT_SHADERS ${SHADER_FILE})
            elseif (SHADER_FILE MATCHES ".*\\.slh")
                list(APPEND SHADER_HEADERS ${SHADER_FILE})
            endif()
        endforeach()

        if (DEFINED SHADER_HEADERS)
            list(REMOVE_DUPLICATES SHADER_HEADERS)
            source_group("${SHADER_LIB}/Headers" FILES ${SHADER_HEADERS})
            list(APPEND ALL_SHADER_HEADERS ${SHADER_HEADERS})
            list(APPEND ALL_SCRIBE_SHADERS ${SHADER_HEADERS})
        endif()

        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace ${SHADER_NAMESPACE} {\n")
        if (DEFINED VERTEX_SHADERS)
            list(REMOVE_DUPLICATES VERTEX_SHADERS)
            source_group("${SHADER_LIB}/Vertex" FILES ${VERTEX_SHADERS})
            list(APPEND ALL_SCRIBE_SHADERS ${VERTEX_SHADERS})
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace vertex { enum {\n")
            foreach(SHADER ${VERTEX_SHADERS}) 
                process_shader_file()
            endforeach()
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "}; } // vertex \n")
        endif()

        if (DEFINED FRAGMENT_SHADERS)
            list(REMOVE_DUPLICATES FRAGMENT_SHADERS)
            source_group("${SHADER_LIB}/Fragment" FILES ${FRAGMENT_SHADERS})
            list(APPEND ALL_SCRIBE_SHADERS ${FRAGMENT_SHADERS})
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace fragment { enum {\n")
            foreach(SHADER ${FRAGMENT_SHADERS}) 
                process_shader_file()
            endforeach()
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "}; } // fragment \n")
        endif()

        if (DEFINED LIB_PROGRAMS)
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "namespace program { enum {\n")
            foreach(LIB_PROGRAM ${LIB_PROGRAMS})
                get_property(LIB_PROGRAM_MAP GLOBAL PROPERTY ${LIB_PROGRAM})
                unpack_map(${LIB_PROGRAM_MAP})
                string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "${LIB_PROGRAM} = (${AUTOSCRIBE_PROGRAM_VERTEX} << 16) |  ${AUTOSCRIBE_PROGRAM_FRAGMENT},\n")
                MATH(EXPR PROGRAM_COUNT "${PROGRAM_COUNT}+1")
                list(APPEND SHADER_ALL_PROGRAMS "${SHADER_NAMESPACE}::program::${LIB_PROGRAM}")
            endforeach()
            string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "}; } // program \n")
        endif()
        string(CONCAT SHADER_ENUMS "${SHADER_ENUMS}" "} // namespace ${SHADER_NAMESPACE}\n")
    endforeach()

    set(SHADER_PROGRAMS_ARRAY "")
    foreach(SHADER_PROGRAM ${SHADER_ALL_PROGRAMS})
        string(CONCAT SHADER_PROGRAMS_ARRAY "${SHADER_PROGRAMS_ARRAY}" "    ${SHADER_PROGRAM},\n")
    endforeach()

    set(SHREFLECT_COMMAND shreflect)
    set(SHREFLECT_DEPENDENCY shreflect)
    if (ANDROID)
        set(SHREFLECT_COMMAND ${NATIVE_SHREFLECT})
        unset(SHREFLECT_DEPENDENCY)
    endif()

    set(SHADER_COUNT 0)
    foreach(COMPILED_SHADER ${COMPILED_SHADERS})
        set(REFLECTED_SHADER "${COMPILED_SHADER}.json")
        list(APPEND COMPILED_SHADER_REFLECTIONS ${REFLECTED_SHADER})
        string(CONCAT SHADER_QRC "${SHADER_QRC}" "<file alias=\"${SHADER_COUNT}\">${COMPILED_SHADER}</file>\n")
        string(CONCAT SHADER_QRC "${SHADER_QRC}" "<file alias=\"${SHADER_COUNT}_reflection\">${REFLECTED_SHADER}</file>\n")
        MATH(EXPR SHADER_COUNT "${SHADER_COUNT}+1")
        add_custom_command(
           OUTPUT ${REFLECTED_SHADER}
           COMMAND ${SHREFLECT_COMMAND} ${COMPILED_SHADER}
           DEPENDS ${SHREFLECT_DEPENDENCY} ${COMPILED_SHADER}
       )
    endforeach()
    
    # Custom targets required to force generation of the shaders via scribe
    add_custom_target(scribe_shaders SOURCES ${ALL_SCRIBE_SHADERS})
    add_custom_target(compiled_shaders SOURCES ${COMPILED_SHADERS})
    add_custom_target(reflected_shaders SOURCES ${COMPILED_SHADER_REFLECTIONS})
    set_target_properties(scribe_shaders PROPERTIES FOLDER "Shaders")
    set_target_properties(compiled_shaders PROPERTIES FOLDER "Shaders")
    set_target_properties(reflected_shaders PROPERTIES FOLDER "Shaders")

    configure_file(
        ShaderEnums.cpp.in 
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.cpp
    )
    configure_file(
        ShaderEnums.h.in 
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.h
    )
    configure_file(
        shaders.qrc.in 
        ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc
    )
    set(AUTOSCRIBE_SHADER_LIB_SRC "${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.h;${CMAKE_CURRENT_BINARY_DIR}/shaders/ShaderEnums.cpp")
    list(APPEND AUTOSCRIBE_SHADER_LIB_SRC ${COMPILED_SHADERS}) 
    set(QT_RESOURCES_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)
    get_property(GLOBAL_SHADER_SOURCES GLOBAL PROPERTY GLOBAL_SHADER_SOURCES)
    list(REMOVE_DUPLICATES GLOBAL_SHADER_SOURCES)
endmacro()
