# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLM)
    if (ANDROID)
        target_include_directories(${TARGET_NAME} PUBLIC "${VCPKG_INSTALL_ROOT}/include")
    else()
        find_package(glm CONFIG REQUIRED)
        target_link_libraries(${TARGET_NAME} glm)
    endif()
    target_compile_definitions(${TARGET_NAME} PUBLIC GLM_FORCE_RADIANS)
    target_compile_definitions(${TARGET_NAME} PUBLIC GLM_ENABLE_EXPERIMENTAL)
    target_compile_definitions(${TARGET_NAME} PUBLIC GLM_FORCE_CTOR_INIT)
endmacro()