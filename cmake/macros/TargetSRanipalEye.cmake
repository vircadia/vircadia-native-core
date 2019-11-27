#
#  Copyright 2019 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_SRANIPAL)

if (WIN32)
    target_include_directories(${TARGET_NAME} SYSTEM PUBLIC "${VCPKG_INSTALL_ROOT}/include")
    find_library(SRANIPAL_LIBRARY NAMES SRanipal PATHS ${VCPKG_INSTALL_ROOT}/lib/ NO_DEFAULT_PATH)
    target_link_libraries(${TARGET_NAME} ${SRANIPAL_LIBRARY})

    find_library(SRANIPAL_LIBRARY NAMES SRanipal PATHS ${VCPKG_INSTALL_ROOT}/lib/ NO_DEFAULT_PATH)
    target_link_libraries(${TARGET_NAME} ${SRANIPAL_LIBRARY})
endif()

endmacro()
