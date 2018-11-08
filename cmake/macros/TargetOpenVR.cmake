#
#  Created by Bradley Austin Davis on 2018/10/24
#  Copyright 2013-2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_OPENVR)
    find_library(OPENVR_LIBRARY_RELEASE NAMES openvr_api PATHS ${VCPKG_INSTALL_ROOT}/lib)
    find_library(OPENVR_LIBRARY_DEBUG NAMES openvr_api PATHS ${VCPKG_INSTALL_ROOT}/debug/lib)
    select_library_configurations(OPENVR)
    target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARY})
endmacro()
