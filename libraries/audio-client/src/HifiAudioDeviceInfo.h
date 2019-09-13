#ifndef hifi_audiodeviceinfo_h
#define hifi_audiodeviceinfo_h

#include <QObject>
#include <QAudioDeviceInfo>
#include <QAudio>
#include <QString>

class HifiAudioDeviceInfo : public QObject {
    Q_OBJECT
    
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
    void setDevice(QAudioDeviceInfo devInfo) {
        _audioDeviceInfo = devInfo;
        setDeviceName();
    }

    QAudioDeviceInfo getDevice() const { return _audioDeviceInfo; }
    QString deviceName() const { return _deviceName; }
    bool isDefault() const { return _isDefault; }
    QAudio::Mode getMode() const { return _mode; }


    HifiAudioDeviceInfo& operator=(const HifiAudioDeviceInfo& other) { 
        _audioDeviceInfo = other.getDevice();
        _deviceName = other.deviceName();
        _mode = other.getMode();
        _isDefault = other.isDefault();
        return *this;
    }

     bool operator==(const HifiAudioDeviceInfo& rhs) const { 
         return _audioDeviceInfo == rhs.getDevice(); 
     }
    
     bool operator!=(const HifiAudioDeviceInfo& rhs) const { 
         return _audioDeviceInfo != rhs.getDevice(); 
     }


private:
    void setDeviceName();

private:
    QAudioDeviceInfo _audioDeviceInfo;
    QString _deviceName;
    bool _isDefault;
    QAudio::Mode _mode;
};

#endif