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
        include_directories(SYSTEM "${BULLET_INCLUDE_DIRS}")
        list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${BULLET_LIBRARIES}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BULLET_PHYSICS")
    endif (BULLET_FOUND)
endmacro(INCLUDE_BULLET)
