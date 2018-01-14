# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLAD)

    if (APPLE)
		set(GLAD_VER "41")
    else()
        if (USE_GLES)
			set(GLAD_VER "32es")
        else()
			set(GLAD_VER "45")
            add_dependency_external_projects(glad45)
        endif()
        if (WIN32)
            find_package(OpenGL REQUIRED)
            target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
            # we don't need the include, because we get everything from GLAD
            #target_include_directories(${TARGET_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
        endif()
    endif()
    set(GLAD "glad${GLAD_VER}")
    string(TOUPPER ${GLAD} GLAD_UPPER)
    add_dependency_external_projects(${GLAD})
    set(GLAD_INCLUDE_DIRS ${${GLAD_UPPER}_INCLUDE_DIRS})
    set(GLAD_LIBRARY ${${GLAD_UPPER}_LIBRARY})

    target_include_directories(${TARGET_NAME} PUBLIC ${GLAD_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${GLAD_LIBRARY})
endmacro()
