macro(AUTO_MTC TARGET ROOT_DIR)
  if (NOT TARGET mtc)
    add_subdirectory("${ROOT_DIR}/tools/mtc" "${ROOT_DIR}/tools/mtc")
  endif ()
  
  set(AUTOMTC_SRC ${TARGET}_automtc.cpp)
  
  file(GLOB INCLUDE_FILES src/*.h)
  
  add_custom_command(OUTPUT ${AUTOMTC_SRC} COMMAND mtc -o ${AUTOMTC_SRC} ${INCLUDE_FILES} DEPENDS mtc ${INCLUDE_FILES})
endmacro()