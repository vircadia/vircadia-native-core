//
//  HifiAudioDeviceInfo.h
//  libraries/audio-client/src
//
//  Created by Amer Cerkic on 9/14/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_audiodeviceinfo_h
#define hifi_audiodeviceinfo_h


#include <QObject>
#include <QAudioDeviceInfo>
#include <QAudio>
#include <QString>

class HifiAudioDeviceInfo : public QObject {
    Q_OBJECT
    
public:
    HifiAudioDeviceInfo() : QObject() {}
    HifiAudioDeviceInfo(const HifiAudioDeviceInfo &deviceInfo) : QObject(){
        _audioDeviceInfo = deviceInfo.getDevice();
        _mode = deviceInfo.getMode();
        _isDefault = deviceInfo.isDefault();
    }

    HifiAudioDeviceInfo(QAudioDeviceInfo deviceInfo, bool isDefault, QAudio::Mode mode) :
        _audioDeviceInfo(deviceInfo),
        _isDefault(isDefault),
        _mode(mode){
    }

    void setMode(QAudio::Mode mode) { _mode = mode; }
    void setIsDefault(bool isDefault = false) { _isDefault = isDefault; }
    void setDevice(QAudioDeviceInfo devInfo);
    QString deviceName() const {
        if (_isDefault) {
            return "default";
        }
        else {
            return _audioDeviceInfo.deviceName();
        }
    }
    QAudioDeviceInfo getDevice() const { return _audioDeviceInfo; }
    bool isDefault() const { return _isDefault; }
    QAudio::Mode getMode() const { return _mode; }

    HifiAudioDeviceInfo& operator=(const HifiAudioDeviceInfo& other);
    bool operator==(const HifiAudioDeviceInfo& rhs) const;
    bool operator!=(const HifiAudioDeviceInfo& rhs) const;

private:
    QAudioDeviceInfo _audioDeviceInfo;
    bool _isDefault { false };
    QAudio::Mode _mode { QAudio::AudioInput };
};

#endif