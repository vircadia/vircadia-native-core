MACRO(SETUP_HIFI_PROJECT TARGET INCLUDE_QT)
    project(${TARGET})
    
    # grab the implemenation and header files
    file(GLOB TARGET_SRCS src/*.cpp src/*.h)
    
    # add the executable
    add_executable(${TARGET} ${TARGET_SRCS})
    
    IF (${INCLUDE_QT})
      find_package(Qt4 REQUIRED QtCore)
      include(${QT_USE_FILE})
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${QT_QTGUI_INCLUDE_DIR}")
    ENDIF()

    target_link_libraries(${TARGET} ${QT_LIBRARIES})
ENDMACRO(SETUP_HIFI_PROJECT _target _include_qt)