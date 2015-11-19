# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_FACESHIFT)
    add_dependency_external_projects(faceshift)
    find_package(Faceshift REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${FACESHIFT_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${FACESHIFT_LIBRARIES})
    add_definitions(-DHAVE_FACESHIFT)
endmacro()