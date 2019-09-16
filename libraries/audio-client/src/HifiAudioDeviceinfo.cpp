#include "HifiAudioDeviceInfo.h"

void HifiAudioDeviceInfo::setDevice(QAudioDeviceInfo devInfo) {
    _audioDeviceInfo = devInfo;
}

HifiAudioDeviceInfo& HifiAudioDeviceInfo::operator=(const HifiAudioDeviceInfo& other) {
    _audioDeviceInfo = other.getDevice();
    _deviceName = other.deviceName();
    _mode = other.getMode();
    _isDefault = other.isDefault();
    return *this;
}

bool HifiAudioDeviceInfo::operator==(const HifiAudioDeviceInfo& rhs) const {
    return _audioDeviceInfo == rhs.getDevice();
}

bool HifiAudioDeviceInfo::operator!=(const HifiAudioDeviceInfo& rhs) const {
    return _audioDeviceInfo != rhs.getDevice();
}

