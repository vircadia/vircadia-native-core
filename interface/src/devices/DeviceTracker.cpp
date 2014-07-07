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

// The singleton managing the connected devices
//template <> DeviceTracker::Singleton DeviceTracker::Singleton::_singleton;
//TemplateSingleton<DeviceTracker::SingletonData>::_singleton;

int DeviceTracker::init() {
    return Singleton::get()->_devicesMap.size();
}
int DeviceTracker::getNumDevices() {
    return Singleton::get()->_devicesMap.size();
}

int DeviceTracker::getDeviceIndex( const Name& name ) {
    auto deviceIt = Singleton::get()->_devicesMap.find( name );
    if ( deviceIt != Singleton::get()->_devicesMap.end() )
        return (*deviceIt).second;
    else
        return -1;
}

DeviceTracker* DeviceTracker::getDevice( const Name& name ) {
    return getDevice( getDeviceIndex( name ) );
}

DeviceTracker* DeviceTracker::getDevice( int deviceNum ) {
    if ( (deviceNum >= 0) && ( deviceNum < Singleton::get()->_devicesVector.size() ) ) {
        return Singleton::get()->_devicesVector[ deviceNum ];
    } else {
        return NULL;
    }
}

int DeviceTracker::registerDevice( const Name& name, DeviceTracker* device ) {
    if ( !device )
        return -1;
    int index = getDeviceIndex( name );
    if ( index >= 0 ) {
        // early exit because device name already taken
        return -2;
    } else {
        index = Singleton::get()->_devicesVector.size();
        Singleton::get()->_devicesMap.insert( SingletonData::Map::value_type( name, index ) );
        Singleton::get()->_devicesVector.push_back( device );
        return index;
    }
}

void DeviceTracker::updateAll()
{
    for ( auto deviceIt = Singleton::get()->_devicesVector.begin(); deviceIt != Singleton::get()->_devicesVector.end(); deviceIt++ ) {
        if ( (*deviceIt) )
            (*deviceIt)->update();
    }
}

// Core features of the Device Tracker

DeviceTracker::DeviceTracker()
{
}

DeviceTracker::~DeviceTracker()
{
}

bool DeviceTracker::isConnected() const {
    return false;
}

void DeviceTracker::update() {
}
