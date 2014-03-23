//
//  AudioDeviceScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/23/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "Application.h"
#include "AudioDeviceScriptingInterface.h"


AudioDeviceScriptingInterface* AudioDeviceScriptingInterface::getInstance() {
    static AudioDeviceScriptingInterface sharedInstance;
    return &sharedInstance;
}

bool AudioDeviceScriptingInterface::setInputDevice(const QString& deviceName) {
    bool result;
    QMetaObject::invokeMethod(Application::getInstance()->getAudio(), "switchInputToAudioDevice",
                        Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(bool, result),
                        Q_ARG(const QString&, deviceName));

    return result;
}

bool AudioDeviceScriptingInterface::setOutputDevice(const QString& deviceName) {
    bool result;
    QMetaObject::invokeMethod(Application::getInstance()->getAudio(), "switchOutputToAudioDevice",
                        Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(bool, result),
                        Q_ARG(const QString&, deviceName));

    return result;
}

QString AudioDeviceScriptingInterface::getInputDevice() {
    return Application::getInstance()->getAudio()->getInputAudioDeviceName();
}

QString AudioDeviceScriptingInterface::getOutputDevice() {
    return Application::getInstance()->getAudio()->getOutputAudioDeviceName();
}
