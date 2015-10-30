# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_SDL2)
    add_dependency_external_projects(sdl2)
    find_package(SDL2 REQUIRED)
    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${SDL2_INCLUDE_DIR})
    target_link_libraries(${TARGET_NAME} ${SDL2_LIBRARY})
    add_definitions(-DHAVE_SDL2)
endmacro()