#
#  Created by Bradley Austin Davis on 2015/10/25
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

#  Windows doesn't need this, and building it currently make Linux unstable.
#  if (NOT WIN32)

if (FALSE)
if (APPLE)

    set(TARGET_NAME oculusLegacy)
    setup_hifi_plugin()
    link_hifi_libraries(shared gl gpu plugins ui ui-plugins display-plugins input-plugins)
    
    include_hifi_library_headers(octree)
    
    add_dependency_external_projects(LibOVR)
    find_package(LibOVR REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${LIBOVR_INCLUDE_DIRS})
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES} ${COCOA_LIBRARY} ${IOKIT_LIBRARY})



endif()
endif()