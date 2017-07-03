#
#  Created by David Rowe on 16 Jun 2017.
#  Copyright 2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

macro(TARGET_LEAPMOTION)
    target_include_directories(${TARGET_NAME} PRIVATE ${LEAPMOTION_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${LEAPMOTION_LIBRARIES})
endmacro()
