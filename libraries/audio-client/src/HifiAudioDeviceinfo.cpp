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
