#ifndef hifi_audiodeviceinfo_h
#define hifi_audiodeviceinfo_h

#include <QObject>
#include <QAudioDeviceInfo>
#include <QAudio>
#include <QString>

class HifiAudioDeviceInfo : public QObject {
    QObject
    
public:
    HifiAudioDeviceInfo() {}
    HifiAudioDeviceInfo(const HifiAudioDeviceInfo &deviceInfo){
        _audioDeviceInfo = deviceInfo.getDevice();
        _mode = deviceInfo.getMode();
        _deviceName = deviceInfo.deviceName();
        _isDefault = deviceInfo.isDefault();
    }

    HifiAudioDeviceInfo(QAudioDeviceInfo deviceInfo, bool isDefault, QAudio::Mode mode) :
        _audioDeviceInfo(deviceInfo), 
        _isDefault(isDefault), 
        _mode(mode) {
        setDeviceName();
    }

    void setMode(QAudio::Mode mode);
    void setIsDefault(bool isDefault = false);
    void setDeviceName(QString name);

    QAudioDeviceInfo getDevice() const { return _audioDeviceInfo; }
    QString deviceName() const { return _deviceName; }
    bool isDefault() const { return isDefault; }
    QAudio::Mode getMode() const { return _mode; }

private:
    void setDeviceName();

private:
    QAudioDeviceInfo _audioDeviceInfo;
    QString _deviceName;
    bool _isDefault;
    QAudio::Mode _mode;
};

#endif