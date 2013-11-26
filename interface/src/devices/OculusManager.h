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

class OculusManager {
public:
    static void connect();
    
    static bool isConnected() { return _isConnected; }
    
    static void reset();
    
    static void getEulerAngles(float& yaw, float& pitch, float& roll);
    
    static void updateYawOffset();
private:    
    static bool _isConnected;
    static OVR::Ptr<OVR::DeviceManager> _deviceManager;
    static OVR::Ptr<OVR::HMDDevice> _hmdDevice;
    static OVR::Ptr<OVR::SensorDevice> _sensorDevice;
    static OVR::SensorFusion* _sensorFusion;
    static float _yawOffset;
};

#endif /* defined(__hifi__OculusManager__) */
