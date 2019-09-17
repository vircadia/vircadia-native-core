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
#include <QCryptographicHash>

void HifiAudioDeviceInfo::setDevice(QAudioDeviceInfo devInfo) {
    _audioDeviceInfo = devInfo;
}

HifiAudioDeviceInfo& HifiAudioDeviceInfo::operator=(const HifiAudioDeviceInfo& other) {
    _audioDeviceInfo = other.getDevice();
    _deviceName = other.deviceName();
    _mode = other.getMode();
    _isDefault = other.isDefault();
    _uniqueId = other.getId();
    return *this;
}

bool HifiAudioDeviceInfo::operator==(const HifiAudioDeviceInfo& rhs) const {
    return _audioDeviceInfo == rhs.getDevice() && getId() == rhs.getId();
}

bool HifiAudioDeviceInfo::operator!=(const HifiAudioDeviceInfo& rhs) const {
    return _audioDeviceInfo != rhs.getDevice() && getId() != rhs.getId();
}

void HifiAudioDeviceInfo::setId(QString name) {
   _uniqueId=QString(QCryptographicHash::hash(name.toUtf8().constData(), QCryptographicHash::Md5).toHex());
}
