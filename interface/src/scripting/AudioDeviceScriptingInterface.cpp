//
//  AudioDeviceScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 3/23/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Audio.h"
#include "AudioDeviceScriptingInterface.h"


AudioDeviceScriptingInterface* AudioDeviceScriptingInterface::getInstance() {
    static AudioDeviceScriptingInterface sharedInstance;
    return &sharedInstance;
}

AudioDeviceScriptingInterface::AudioDeviceScriptingInterface() {
    connect(DependencyManager::get<Audio>().data(), &Audio::muteToggled,
            this, &AudioDeviceScriptingInterface::muteToggled);
    connect(DependencyManager::get<Audio>().data(), &Audio::deviceChanged,
        this, &AudioDeviceScriptingInterface::deviceChanged);
}

bool AudioDeviceScriptingInterface::setInputDevice(const QString& deviceName) {
    bool result;
    QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "switchInputToAudioDevice",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(const QString&, deviceName));

    return result;
}

bool AudioDeviceScriptingInterface::setOutputDevice(const QString& deviceName) {
    bool result;
    QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "switchOutputToAudioDevice",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(const QString&, deviceName));

    return result;
}

QString AudioDeviceScriptingInterface::getInputDevice() {
    return DependencyManager::get<Audio>()->getDeviceName(QAudio::AudioInput);
}

QString AudioDeviceScriptingInterface::getOutputDevice() {
    return DependencyManager::get<Audio>()->getDeviceName(QAudio::AudioOutput);
}

QString AudioDeviceScriptingInterface::getDefaultInputDevice() {
    return DependencyManager::get<Audio>()->getDefaultDeviceName(QAudio::AudioInput);
}

QString AudioDeviceScriptingInterface::getDefaultOutputDevice() {
    return DependencyManager::get<Audio>()->getDefaultDeviceName(QAudio::AudioOutput);
}

QVector<QString> AudioDeviceScriptingInterface::getInputDevices() {
    return DependencyManager::get<Audio>()->getDeviceNames(QAudio::AudioInput);
}

QVector<QString> AudioDeviceScriptingInterface::getOutputDevices() {
    return DependencyManager::get<Audio>()->getDeviceNames(QAudio::AudioOutput);
}


float AudioDeviceScriptingInterface::getInputVolume() {
    return DependencyManager::get<Audio>()->getInputVolume();
}

void AudioDeviceScriptingInterface::setInputVolume(float volume) {
    DependencyManager::get<Audio>()->setInputVolume(volume);
}

void AudioDeviceScriptingInterface::setReverb(bool reverb) {
    DependencyManager::get<Audio>()->setReverb(reverb);
}

void AudioDeviceScriptingInterface::setReverbOptions(const AudioEffectOptions* options) {
    DependencyManager::get<Audio>()->setReverbOptions(options);
}

void AudioDeviceScriptingInterface::toggleMute() {
    DependencyManager::get<Audio>()->toggleMute();
}

bool AudioDeviceScriptingInterface::getMuted() {
    return DependencyManager::get<Audio>()->isMuted();
}
