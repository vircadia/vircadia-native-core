#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Olivier Prat on 2019/03/26
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_OPENEXR)

if (NOT ANDROID)
   	# using VCPKG for OPENEXR
    find_package(OpenEXR REQUIRED)

    include_directories(SYSTEM "${OpenEXR_INCLUDE_DIRS}")
    target_link_libraries(${TARGET_NAME} ${OpenEXR_LIBRARIES})
    target_compile_definitions(${TARGET_NAME} PUBLIC OPENEXR_DLL)
endif()

endmacro()

