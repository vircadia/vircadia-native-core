# 
#  IncludeBullet.cmake
# 
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(INCLUDE_BULLET)
    find_package(Bullet REQUIRED)
    include_directories("${BULLET_INCLUDE_DIRS}")
endmacro(INCLUDE_BULLET)
