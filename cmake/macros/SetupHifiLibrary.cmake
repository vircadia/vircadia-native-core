MACRO(SETUP_HIFI_LIBRARY TARGET)
    project(${TARGET_NAME})

    # grab the implemenation and header files
    file(GLOB LIB_SRCS src/*.h src/*.cpp)

    # create a library and set the property so it can be referenced later
    add_library(${TARGET_NAME} ${LIB_SRCS})
ENDMACRO(SETUP_HIFI_LIBRARY _target)