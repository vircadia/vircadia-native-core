MACRO(LINK_HIFI_VOXELLIB_LIBRARY TARGET)
    if (NOT TARGET HifiVoxelLib)
        add_subdirectory(../voxellib ../voxellib)
    endif (NOT TARGET HifiVoxelLib)
    
    include_directories(../voxellib/src)
    get_directory_property(HIFI_VOXELLIB_LIBRARY DIRECTORY ../voxellib DEFINITION HIFI_VOXELLIB_LIBRARY)
    add_dependencies(${TARGET} ${HIFI_VOXELLIB_LIBRARY})
    target_link_libraries(${TARGET} ${HIFI_VOXELLIB_LIBRARY})

    if (APPLE)
      # link in required OS X framework
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreServices")
    endif (APPLE)
    
ENDMACRO(LINK_HIFI_VOXELLIB_LIBRARY _target)
