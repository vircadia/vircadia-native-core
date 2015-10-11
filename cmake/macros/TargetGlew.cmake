# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLEW)
    add_dependency_external_projects(glew)
    find_package(GLEW REQUIRED)
    add_definitions(-DGLEW_STATIC) 
    target_include_directories(${TARGET_NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARY})
endmacro()