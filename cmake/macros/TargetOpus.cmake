#
#  Created by Michael Bailey on 12/20/2019
#  Copyright 2019 Michael Bailey
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_opus)
    find_library(OPUS_LIBRARY_RELEASE NAMES opus PATHS ${VCPKG_INSTALL_ROOT}/lib)
    find_library(OPUS_LIBRARY_DEBUG NAMES opus PATHS ${VCPKG_INSTALL_ROOT}/debug/lib)
    select_library_configurations(OPUS)
    target_link_libraries(${TARGET_NAME} ${OPUS_LIBRARY})
endmacro()
