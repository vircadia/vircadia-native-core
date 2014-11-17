//
//  main.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 11/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <android_native_app_glue.h>
#include <android/log.h>

#define APPNAME "Interface"

void android_main(struct android_app* state) {
    app_dummy();
    
    __android_log_print(ANDROID_LOG_INFO, APPNAME, "GearVR Interface, reporting for duty");
  
    ANativeActivity_finish(state->activity);
}