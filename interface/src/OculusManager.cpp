//
//  OculusManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "OculusManager.h"
#include <glm/glm.hpp>

bool OculusManager::_isConnected = false;
Ptr<DeviceManager> OculusManager::_deviceManager;
Ptr<HMDDevice> OculusManager::_hmdDevice;
Ptr<SensorDevice> OculusManager::_sensorDevice;
SensorFusion OculusManager::_sensorFusion;

void OculusManager::connect() {
    System::Init();
    _deviceManager = *DeviceManager::Create();
    _hmdDevice = *_deviceManager->EnumerateDevices<HMDDevice>().CreateDevice();

    if (_hmdDevice) {
        _isConnected = true;
        
        _sensorDevice = *_hmdDevice->GetSensor();
        _sensorFusion.AttachToSensor(_sensorDevice);
        
        // default the yaw to the current orientation
        _sensorFusion.SetMagReference();
    }
}

void OculusManager::getEulerAngles(float& yaw, float& pitch, float& roll) {
    _sensorFusion.GetOrientation().GetEulerAngles<Axis_Y, Axis_X, Axis_Z, Rotate_CW, Handed_R>(&yaw, &pitch, &roll);
    
    // convert each angle to degrees
    yaw = glm::degrees(yaw);
    pitch = glm::degrees(pitch);
    roll = glm::degrees(roll);
}

