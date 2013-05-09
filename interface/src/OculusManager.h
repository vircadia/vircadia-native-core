//
//  OculusManager.h
//  hifi
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__OculusManager__
#define __hifi__OculusManager__

#include <iostream>
#include <OVR.h>

using namespace OVR;

class OculusManager {
public:
    static void connect();
private:
    static SensorFusion* _sensorFusion;
};

#endif /* defined(__hifi__OculusManager__) */
