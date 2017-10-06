#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_TBB)

if (ANDROID)
    set(TBB_INSTALL_DIR C:/tbb-2018/built)
    set(TBB_LIBRARY ${HIFI_ANDROID_PRECOMPILED}/libtbb.so CACHE FILEPATH "TBB library location")
    set(TBB_MALLOC_LIBRARY ${HIFI_ANDROID_PRECOMPILED}/libtbbmalloc.so CACHE FILEPATH "TBB malloc library location")
    set(TBB_INCLUDE_DIRS ${TBB_INSTALL_DIR}/include CACHE TYPE "List of tbb include directories" CACHE FILEPATH "TBB includes location")
    set(TBB_LIBRARIES ${TBB_LIBRARY} ${TBB_MALLOC_LIBRARY})
else()
    add_dependency_external_projects(tbb)
    find_package(TBB REQUIRED)
endif()

target_link_libraries(${TARGET_NAME} ${TBB_LIBRARIES})
target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${TBB_INCLUDE_DIRS})

endmacro()
