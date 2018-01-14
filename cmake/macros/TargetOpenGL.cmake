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
    else()
        target_glad()
        target_nsight()
    endif()
endmacro()
