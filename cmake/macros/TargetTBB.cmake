#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_TBB)

if (ANDROID)
    set(TBB_INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/tbb)
    set(TBB_INCLUDE_DIRS ${TBB_INSTALL_DIR}/include CACHE FILEPATH "TBB includes location")
    set(TBB_LIBRARY ${TBB_INSTALL_DIR}/lib/release/libtbb.so CACHE FILEPATH "TBB library location")
    set(TBB_MALLOC_LIBRARY ${TBB_INSTALL_DIR}/lib/release/libtbbmalloc.so CACHE FILEPATH "TBB malloc library location")
    set(TBB_LIBRARIES ${TBB_LIBRARY} ${TBB_MALLOC_LIBRARY})
    target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${TBB_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${TBB_LIBRARIES})
else()
   	# using VCPKG for TBB
    find_package(TBB CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} TBB::tbb)
endif()

endmacro()
