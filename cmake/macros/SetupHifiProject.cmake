MACRO(SETUP_HIFI_PROJECT TARGET INCLUDE_QT)
    project(${TARGET})
    
    # grab the implemenation and header files
    file(GLOB TARGET_SRCS src/*.cpp src/*.h)
    
    # add the executable
    add_executable(${TARGET} ${TARGET_SRCS})
    
    IF (${INCLUDE_QT})
      find_package(Qt5Core REQUIRED)
      qt5_use_modules(${TARGET} Core)
    ENDIF()

    target_link_libraries(${TARGET} ${QT_LIBRARIES})
ENDMACRO(SETUP_HIFI_PROJECT _target _include_qt)