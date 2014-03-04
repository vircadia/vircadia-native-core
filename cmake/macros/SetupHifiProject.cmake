macro(SETUP_HIFI_PROJECT TARGET INCLUDE_QT)
    project(${TARGET})
    
    # grab the implemenation and header files
    file(GLOB TARGET_SRCS src/*)
    
    file(GLOB SRC_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/*)
    foreach(DIR ${SRC_SUBDIRS})
      if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/${DIR}")
        FILE(GLOB DIR_CONTENTS "src/${DIR}/*")
        SET(TARGET_SRCS ${TARGET_SRCS} "${DIR_CONTENTS}")
      endif()
    endforeach()
    
    # add the executable, include additional optional sources
    add_executable(${TARGET} ${TARGET_SRCS} ${ARGN})
    
    IF (${INCLUDE_QT})
      find_package(Qt5Core REQUIRED)
      qt5_use_modules(${TARGET} Core)
    ENDIF()

    target_link_libraries(${TARGET} ${QT_LIBRARIES})
endmacro()