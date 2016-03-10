//
//  DeviceTracker.cpp
//  interface/src/devices
//
//  Created by Sam Cake on 6/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeviceTracker.h"

DeviceTracker::SingletonData::~SingletonData() {
    // Destroy all the device registered
    //TODO C++11 for (auto device = _devicesVector.begin(); device != _devicesVector.end(); device++) {
    for (Vector::iterator device = _devicesVector.begin(); device != _devicesVector.end(); device++) {
        delete (*device);
    }
}

int DeviceTracker::getNumDevices() {
    return (int)Singleton::get()->_devicesMap.size();
}

DeviceTracker::ID DeviceTracker::getDeviceID(const Name& name) {
    //TODO C++11 auto deviceIt = Singleton::get()->_devicesMap.find(name);
    Map::iterator deviceIt = Singleton::get()->_devicesMap.find(name);
    if (deviceIt != Singleton::get()->_devicesMap.end()) {
        return (*deviceIt).second;
    } else {
        return INVALID_DEVICE;
    }
}

DeviceTracker* DeviceTracker::getDevice(const Name& name) {
    return getDevice(getDeviceID(name));
}

DeviceTracker* DeviceTracker::getDevice(DeviceTracker::ID deviceID) {
    if ((deviceID >= 0) && (deviceID < (int)(Singleton::get()->_devicesVector.size()))) {
        return Singleton::get()->_devicesVector[ deviceID ];
    } else {
        return NULL;
    }
}

DeviceTracker::ID DeviceTracker::registerDevice(const Name& name, DeviceTracker* device) {
    // Check that the device exists, if not exit
    if (!device) {
        return INVALID_DEVICE;
    }

    // Look if the name is not already used
    ID deviceID = getDeviceID(name);
    if (deviceID >= 0) {
        return INVALID_DEVICE_NAME;
    }

    // Good to register the device
    deviceID = (ID)Singleton::get()->_devicesVector.size();
    Singleton::get()->_devicesMap.insert(Map::value_type(name, deviceID));
    Singleton::get()->_devicesVector.push_back(device);
    device->assignIDAndName(deviceID, name);

    return deviceID;
}

void DeviceTracker::destroyDevice(const Name& name) {
    DeviceTracker::ID deviceID = getDeviceID(name);
    if (deviceID != INVALID_DEVICE) {
        delete Singleton::get()->_devicesVector[getDeviceID(name)];
        Singleton::get()->_devicesVector[getDeviceID(name)] = nullptr;
    }
}

void DeviceTracker::updateAll() {
    //TODO C++11 for (auto deviceIt = Singleton::get()->_devicesVector.begin(); deviceIt != Singleton::get()->_devicesVector.end(); deviceIt++) {
    for (Vector::iterator deviceIt = Singleton::get()->_devicesVector.begin(); deviceIt != Singleton::get()->_devicesVector.end(); deviceIt++) {
        if ((*deviceIt))
            (*deviceIt)->update();
    }
}

// Core features of the Device Tracker
DeviceTracker::DeviceTracker() :
    _ID(INVALID_DEVICE),
    _name("Unkown")
{
}

DeviceTracker::~DeviceTracker() {
}

void DeviceTracker::update() {
}
