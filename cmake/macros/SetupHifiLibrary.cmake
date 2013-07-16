MACRO(SETUP_HIFI_LIBRARY TARGET)
    project(${TARGET})

    # grab the implemenation and header files
    file(GLOB LIB_SRCS src/*.h src/*.cpp)

    # create a library and set the property so it can be referenced later
    add_library(${TARGET} ${LIB_SRCS})
    
    find_package(Qt4 REQUIRED QtCore)
    include(${QT_USE_FILE})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${QT_QTGUI_INCLUDE_DIR}")

    target_link_libraries(${TARGET} ${QT_LIBRARIES})
ENDMACRO(SETUP_HIFI_LIBRARY _target)