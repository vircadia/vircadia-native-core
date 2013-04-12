MACRO(SETUP_HIFI_PROJECT TARGET)
    project(${TARGET})
    
    # grab the implemenation and header files
    file(GLOB TARGET_SRCS src/*.cpp src/*.h)
    
    # add the executable
    add_executable(${TARGET} ${TARGET_SRCS})
ENDMACRO(SETUP_HIFI_PROJECT _target)