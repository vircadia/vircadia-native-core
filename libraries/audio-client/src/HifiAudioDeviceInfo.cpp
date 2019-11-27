//
//  HifiAudioDeviceInfo.cpp
//  libraries/audio-client/src
//
//  Created by Amer Cerkic on 9/14/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "HifiAudioDeviceInfo.h"

const QString HifiAudioDeviceInfo::DEFAULT_DEVICE_NAME = "default ";

void HifiAudioDeviceInfo::setDevice(QAudioDeviceInfo devInfo) {
    _audioDeviceInfo = devInfo;
}

HifiAudioDeviceInfo& HifiAudioDeviceInfo::operator=(const HifiAudioDeviceInfo& other) {
    _audioDeviceInfo = other.getDevice();
    _mode = other.getMode();
    _isDefault = other.isDefault();
    _deviceType = other.getDeviceType();
    _debugName = other.getDevice().deviceName();
    return *this;
}

bool HifiAudioDeviceInfo::operator==(const HifiAudioDeviceInfo& rhs) const {
    //Does the QAudioDeviceinfo match as well as is this the default device or 
    return  getDevice() == rhs.getDevice() && isDefault() == rhs.isDefault();
}
bool HifiAudioDeviceInfo::operator!=(const HifiAudioDeviceInfo& rhs) const {
    return  getDevice() != rhs.getDevice() || isDefault() != rhs.isDefault();
}