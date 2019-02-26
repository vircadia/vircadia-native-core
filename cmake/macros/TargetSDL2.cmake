# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_SDL2)
   	# using VCPKG for SDL2
    find_package(SDL2 CONFIG REQUIRED)
    if (WIN32)
        target_link_libraries(${TARGET_NAME} SDL2::SDL2)
    else()
        target_link_libraries(${TARGET_NAME} SDL2::SDL2-static)
    endif()
    add_definitions(-DHAVE_SDL2)
endmacro()
