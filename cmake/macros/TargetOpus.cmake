#
#  Created by Michael Bailey on 12/20/2019
#  Copyright 2019 Michael Bailey
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_opus)
    if (ANDROID)
        # no idea if this is correct
        target_link_libraries(${TARGET_NAME})
    else()
        # using VCPKG for opus
        find_package(OPUS REQUIRED)
        target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} ${OPUS_LIBRARIES})
    endif()
endmacro()
