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

#include "AudioClient.h"
#include "AudioDeviceScriptingInterface.h"


AudioDeviceScriptingInterface* AudioDeviceScriptingInterface::getInstance() {
    static AudioDeviceScriptingInterface sharedInstance;
    return &sharedInstance;
}

AudioDeviceScriptingInterface::AudioDeviceScriptingInterface() {
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::muteToggled,
            this, &AudioDeviceScriptingInterface::muteToggled);
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::deviceChanged,
        this, &AudioDeviceScriptingInterface::deviceChanged);
}

bool AudioDeviceScriptingInterface::setInputDevice(const QString& deviceName) {
    bool result;
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchInputToAudioDevice",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(const QString&, deviceName));

    return result;
}

bool AudioDeviceScriptingInterface::setOutputDevice(const QString& deviceName) {
    bool result;
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchOutputToAudioDevice",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(const QString&, deviceName));

    return result;
}

QString AudioDeviceScriptingInterface::getInputDevice() {
    return DependencyManager::get<AudioClient>()->getDeviceName(QAudio::AudioInput);
}

QString AudioDeviceScriptingInterface::getOutputDevice() {
    return DependencyManager::get<AudioClient>()->getDeviceName(QAudio::AudioOutput);
}

QString AudioDeviceScriptingInterface::getDefaultInputDevice() {
    return DependencyManager::get<AudioClient>()->getDefaultDeviceName(QAudio::AudioInput);
}

QString AudioDeviceScriptingInterface::getDefaultOutputDevice() {
    return DependencyManager::get<AudioClient>()->getDefaultDeviceName(QAudio::AudioOutput);
}

QVector<QString> AudioDeviceScriptingInterface::getInputDevices() {
    return DependencyManager::get<AudioClient>()->getDeviceNames(QAudio::AudioInput);
}

QVector<QString> AudioDeviceScriptingInterface::getOutputDevices() {
    return DependencyManager::get<AudioClient>()->getDeviceNames(QAudio::AudioOutput);
}


float AudioDeviceScriptingInterface::getInputVolume() {
    return DependencyManager::get<AudioClient>()->getInputVolume();
}

void AudioDeviceScriptingInterface::setInputVolume(float volume) {
    DependencyManager::get<AudioClient>()->setInputVolume(volume);
}

void AudioDeviceScriptingInterface::setReverb(bool reverb) {
    DependencyManager::get<AudioClient>()->setReverb(reverb);
}

void AudioDeviceScriptingInterface::setReverbOptions(const AudioEffectOptions* options) {
    DependencyManager::get<AudioClient>()->setReverbOptions(options);
}

void AudioDeviceScriptingInterface::toggleMute() {
    DependencyManager::get<AudioClient>()->toggleMute();
}

bool AudioDeviceScriptingInterface::getMuted() {
    return DependencyManager::get<AudioClient>()->isMuted();
}
