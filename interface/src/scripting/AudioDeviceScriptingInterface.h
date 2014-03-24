//
//  AudioDeviceScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioDeviceScriptingInterface__
#define __hifi__AudioDeviceScriptingInterface__

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
};

#endif /* defined(__hifi__AudioDeviceScriptingInterface__) */
