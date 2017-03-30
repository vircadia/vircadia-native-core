//
//  AudioDeviceScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 3/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioDeviceScriptingInterface_h
#define hifi_AudioDeviceScriptingInterface_h

#include <QObject>
#include <QString>
#include <QVector>

class AudioEffectOptions;

class AudioDeviceScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(QStringList inputAudioDevices READ inputAudioDevices NOTIFY inputAudioDevicesChanged)
    Q_PROPERTY(QStringList outputAudioDevices READ outputAudioDevices NOTIFY outputAudioDevicesChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)

public:
    static AudioDeviceScriptingInterface* getInstance();

    QStringList inputAudioDevices() const;
    QStringList outputAudioDevices() const;
    bool muted();

public slots:
    bool setInputDevice(const QString& deviceName);
    bool setOutputDevice(const QString& deviceName);

    QString getInputDevice();
    QString getOutputDevice();

    QString getDefaultInputDevice();
    QString getDefaultOutputDevice();

    QVector<QString> getInputDevices();
    QVector<QString> getOutputDevices();

    float getInputVolume();
    void setInputVolume(float volume);
    void setReverb(bool reverb);
    void setReverbOptions(const AudioEffectOptions* options);

    bool getMuted();
    void toggleMute();
    
    void setMuted(bool muted);

private:
    AudioDeviceScriptingInterface();

signals:
    void muteToggled();
    void deviceChanged();
    void mutedChanged(bool muted);
    void inputAudioDevicesChanged(QStringList inputAudioDevices);
    void outputAudioDevicesChanged(QStringList outputAudioDevices);
};

#endif // hifi_AudioDeviceScriptingInterface_h
