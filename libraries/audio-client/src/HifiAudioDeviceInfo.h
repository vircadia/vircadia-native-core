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
    HifiAudioDeviceInfo(const HifiAudioDeviceInfo &deviceInfo){
        _audioDeviceInfo = deviceInfo.getDevice();
        _mode = deviceInfo.getMode();
        _deviceName = deviceInfo.deviceName();
        _isDefault = deviceInfo.isDefault();
    }

    HifiAudioDeviceInfo(QAudioDeviceInfo deviceInfo, bool isDefault, QAudio::Mode mode) :
        _isDefault(isDefault),
        _mode(mode),
        _audioDeviceInfo(deviceInfo),
        _deviceName(deviceInfo.deviceName()){
    }

    void setMode(QAudio::Mode mode) { _mode = mode; }
    void setIsDefault(bool isDefault = false) { _isDefault = isDefault; }

    void setDeviceName(QString name) {
        _deviceName = name;
    }

    void setDevice(QAudioDeviceInfo devInfo);

    QAudioDeviceInfo getDevice() const { return _audioDeviceInfo; }
    QString deviceName() const { return _deviceName; }
    bool isDefault() const { return _isDefault; }
    QAudio::Mode getMode() const { return _mode; }

    HifiAudioDeviceInfo& operator=(const HifiAudioDeviceInfo& other);
    bool operator==(const HifiAudioDeviceInfo& rhs) const;
    bool operator!=(const HifiAudioDeviceInfo& rhs) const;

private:
    QAudioDeviceInfo _audioDeviceInfo;
    QString _deviceName{ "" };
    bool _isDefault { false };
    QAudio::Mode _mode { QAudio::AudioInput };
};

#endif