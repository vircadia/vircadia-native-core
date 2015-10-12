# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_OGLPLUS)
    # our OGL plus setup requires glew
    target_glew()
    
    # our OGL plus setup requires boostconfig
    add_dependency_external_projects(boostconfig)
    find_package(BoostConfig REQUIRED)
    target_include_directories(${TARGET_NAME} PUBLIC ${BOOSTCONFIG_INCLUDE_DIRS})

    
    add_dependency_external_projects(oglplus)
    find_package(OGLPLUS REQUIRED)
    target_include_directories(${TARGET_NAME} PUBLIC ${OGLPLUS_INCLUDE_DIRS})
endmacro()