MACRO(SETUP_HIFI_LIBRARY TARGET)
    project(${TARGET})

    # grab the implemenation and header files
    file(GLOB LIB_SRCS src/*.h src/*.cpp)
    set(LIB_SRCS ${LIB_SRCS} ${WRAPPED_SRCS})

    # create a library and set the property so it can be referenced later
    add_library(${TARGET} ${LIB_SRCS})
    
    find_package(Qt5Core REQUIRED)
    qt5_use_modules(${TARGET} Core)

    target_link_libraries(${TARGET} ${QT_LIBRARIES})
ENDMACRO(SETUP_HIFI_LIBRARY _target)