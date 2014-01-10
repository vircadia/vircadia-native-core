macro(AUTO_MTC TARGET ROOT_DIR)
    if (NOT TARGET mtc)
        add_subdirectory(${ROOT_DIR}/tools/mtc ${ROOT_DIR}/tools/mtc)
    endif (NOT TARGET mtc)
    
    file(GLOB INCLUDE_FILES src/*.h)
    
    add_custom_command(OUTPUT ${TARGET}_automtc.cpp COMMAND mtc -o ${TARGET}_automtc.cpp
        ${INCLUDE_FILES} DEPENDS mtc ${INCLUDE_FILES})
    
    find_package(Qt5Core REQUIRED)
    
    add_library(${TARGET}_automtc STATIC ${TARGET}_automtc.cpp)
    
    qt5_use_modules(${TARGET}_automtc Core)
    
    target_link_libraries(${TARGET} ${TARGET}_automtc)
    
endmacro()


