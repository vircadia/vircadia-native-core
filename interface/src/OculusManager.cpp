//
//  OculusManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "OculusManager.h"

bool OculusManager::_isConnected = false;
SensorFusion* OculusManager::_sensorFusion = NULL;

void OculusManager::connect() {
    System::Init();
    Ptr<DeviceManager> deviceManager = *DeviceManager::Create();
    Ptr<HMDDevice> oculus = *deviceManager->EnumerateDevices<HMDDevice>().CreateDevice();

    if (oculus) {
        _isConnected = true;
        
        Ptr<SensorDevice> sensor = *oculus->GetSensor();
        _sensorFusion = new SensorFusion(sensor);
    }
}

void OculusManager::setFloatToAxisAngle(Vector3<float>& axis, float& floatToSet) {
    Quatf orientation = _sensorFusion->GetOrientation();
    orientation.GetAxisAngle(&axis, &floatToSet);
}

float OculusManager::getYaw() {
    float yaw = 0.f;
    Vector3<float> yAxis = Vector3<float>(0.f, 1.f, 0.f);
    
    setFloatToAxisAngle(yAxis, yaw);
    return yaw;    
}

float OculusManager::getPitch() {
    float pitch = 0.f;
    Vector3<float> xAxis = Vector3<float>(1.f, 0.f, 0.f);
    
    setFloatToAxisAngle(xAxis, pitch);
    return pitch;
}

float OculusManager::getRoll() {
    float roll = 0.f;
    Vector3<float> zAxis = Vector3<float>(0.f, 0.f, 1.f);
    
    setFloatToAxisAngle(zAxis, roll);
    return roll;
}

