# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_ZLIB)

    if (WIN32)
        add_dependency_external_projects(zlib)
    endif()

    find_package(ZLIB REQUIRED)

    if (WIN32)
        add_paths_to_fixup_libs(${ZLIB_DLL_PATH})
    endif()

    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})
endmacro()
