macro(target_egl)
    find_library(EGL EGL)
    target_link_libraries(${TARGET_NAME} ${EGL})
endmacro()
