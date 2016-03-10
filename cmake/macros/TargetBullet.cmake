# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_BULLET)
    add_dependency_external_projects(bullet)
    find_package(Bullet REQUIRED)
    # perform the system include hack for OS X to ignore warnings
    if (APPLE)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${BULLET_INCLUDE_DIRS}")
    else()
      target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${BULLET_INCLUDE_DIRS})
    endif()
    target_link_libraries(${TARGET_NAME} ${BULLET_LIBRARIES})
endmacro()
