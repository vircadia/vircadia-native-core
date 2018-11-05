# 
#  Created by Bradley Austin Davis on 2018/07/22
#  Copyright 2013-2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_JSON)
    add_dependency_external_projects(json)
    find_package(JSON REQUIRED)
    target_include_directories(${TARGET_NAME} PUBLIC ${JSON_INCLUDE_DIRS})
endmacro()