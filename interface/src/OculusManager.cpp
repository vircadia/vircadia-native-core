//
//  OculusManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "OculusManager.h"

SensorFusion* OculusManager::_sensorFusion = NULL;

void OculusManager::connect() {
    System::Init();
    Ptr<DeviceManager> deviceManager = *DeviceManager::Create();
    Ptr<HMDDevice> oculus = *deviceManager->EnumerateDevices<HMDDevice>().CreateDevice();

    if (oculus) {
        Ptr<SensorDevice> sensor = *oculus->GetSensor();
        _sensorFusion = new SensorFusion(sensor);
    }
}