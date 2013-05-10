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
    
    static bool isConnected() { return _isConnected; }
    
    static void getEulerAngles(float& yaw, float& pitch, float& roll);
private:    
    static bool _isConnected;
    static Ptr<DeviceManager> _deviceManager;
    static Ptr<HMDDevice> _hmdDevice;
    static Ptr<SensorDevice> _sensorDevice;
    static SensorFusion _sensorFusion;
};

#endif /* defined(__hifi__OculusManager__) */
