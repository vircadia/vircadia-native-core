# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_OPENGL)
    if (ANDROID)
        find_library(EGL EGL)
        find_library(OpenGLES3 GLESv3)
        list(APPEND IGNORE_COPY_LIBS ${OpenGLES3} ${OpenGLES2} ${EGL})
        target_link_libraries(${TARGET_NAME} ${OpenGLES3} ${EGL})
    elseif (USE_GLES)
        set(ANGLE_INCLUDE_DIR "${QT_DIR}/include/QtANGLE")
        set(ANGLE_LIB_DIR "${QT_DIR}/lib")
        find_library(EGL_LIBRARY_RELEASE "libEGL" HINTS ${ANGLE_LIB_DIR})
        find_library(EGL_LIBRARY_DEBUG "libEGLd" HINTS ${ANGLE_LIB_DIR})
        find_library(GLES_LIBRARY_RELEASE "libGLESv2" HINTS ${ANGLE_LIB_DIR})
        find_library(GLES_LIBRARY_DEBUG "libGLESv2d" HINTS ${ANGLE_LIB_DIR})
        include(SelectLibraryConfigurations)
        select_library_configurations(EGL)
        select_library_configurations(GLES)
        message("QQQ ${GLES_LIBRARIES}")
        target_link_libraries(${TARGET_NAME} ${EGL_LIBRARIES})
        target_link_libraries(${TARGET_NAME} ${GLES_LIBRARIES})
        target_include_directories(${TARGET_NAME} PUBLIC "${ANGLE_INCLUDE_DIR}")
    elseif (APPLE)
        # link in required OS X frameworks and include the right GL headers
        find_library(OpenGL OpenGL)
        target_link_libraries(${TARGET_NAME} ${OpenGL})
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
        target_include_directories(${TARGET_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
    endif()
    target_nsight()
    target_glew()
endmacro()
