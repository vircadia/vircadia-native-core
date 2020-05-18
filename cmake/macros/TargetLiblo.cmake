macro(target_liblo)
    find_library(LIBLO LIBLO)
    target_link_libraries(${TARGET_NAME} ${LIBLO})
endmacro()