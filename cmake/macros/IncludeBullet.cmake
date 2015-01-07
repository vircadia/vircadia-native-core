# 
#  IncludeBullet.cmake
# 
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(INCLUDE_BULLET)
    find_package(Bullet)
    if (BULLET_FOUND)
        include_directories("${BULLET_INCLUDE_DIRS}")
        if (APPLE OR UNIX)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BULLET_PHYSICS -isystem ${BULLET_INCLUDE_DIRS}")
        endif ()
    endif (BULLET_FOUND)
endmacro(INCLUDE_BULLET)
