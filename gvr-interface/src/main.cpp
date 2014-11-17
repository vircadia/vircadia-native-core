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

#include <NodeList.h>

// usage of log
#define APP_NAME "Interface"

// usage of log
#define LOGINFO(x...) __android_log_print(ANDROID_LOG_INFO, APP_NAME, x)

// handle commands
static void custom_handle_cmd(struct android_app* app, int32_t cmd) {
    switch(cmd) {
       case APP_CMD_INIT_WINDOW:
        LOGINFO("GearVR Interface, reporting for duty");
        break;
    }
}

void android_main(struct android_app* state) {
    // Make sure glue isn't stripped.
    app_dummy();
     
    int events;
    
    // set up so when commands happen we call our custom handler
    state->onAppCmd = custom_handle_cmd; 
    
    NodeList* nodeList = NodeList::createInstance(NodeType::Agent);
        
    while (1) {
        struct android_poll_source* source;

        // we block for events 
        while (ALooper_pollAll(-1, NULL, &events, (void**)&source) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                LOGINFO("We are exiting");
                return;
            }
        }  
    }
}