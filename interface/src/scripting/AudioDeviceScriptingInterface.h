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

#include <QDebug>
#include <QObject>
#include <QString>

#include "Application.h"

class AudioDeviceScriptingInterface : public QObject {
    Q_OBJECT
    AudioDeviceScriptingInterface() { };
public:
    static AudioDeviceScriptingInterface* getInstance();

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
};

#endif // hifi_AudioDeviceScriptingInterface_h
