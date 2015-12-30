#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Anthony J. Thibault on 2015/12/21
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_NEURON)
    # Neuron data reader is only available on these platforms
    if (WIN32 OR APPLE)
        add_dependency_external_projects(neuron)
        find_package(Neuron REQUIRED)
        target_include_directories(${TARGET_NAME} PRIVATE ${NEURON_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} ${NEURON_LIBRARIES})
        add_definitions(-DHAVE_NEURON)
    endif(WIN32 OR APPLE)
endmacro()
