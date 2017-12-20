# 
#  Created by Bradley Austin Davis on 2017/11/28
#  Copyright 2013-2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GOOGLEVR)
    if (ANDROID)
        set(GVR_ROOT "${HIFI_ANDROID_PRECOMPILED}/gvr/gvr-android-sdk-1.101.0/")
    	target_include_directories(native-lib PRIVATE  "${GVR_ROOT}/libraries/headers")
        target_link_libraries(native-lib "${GVR_ROOT}/libraries/libgvr.so")
    endif()
endmacro()
