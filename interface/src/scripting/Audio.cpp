//
//  Audio.cpp
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 28/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Audio.h"

#include "Application.h"
#include "AudioClient.h"
#include "ui/AvatarInputs.h"

using namespace scripting;

QString Audio::AUDIO { "Audio" };
QString Audio::DESKTOP { "Desktop" };
QString Audio::HMD { "VR" };

Setting::Handle<bool> enableNoiseReductionSetting { QStringList { Audio::AUDIO, "NoiseReduction" }, true };

Audio::Audio() {
    auto client = DependencyManager::get<AudioClient>();
    connect(client.data(), &AudioClient::muteToggled, this, &Audio::onChangedMuted);

    connect(&_devices._inputs, &AudioDeviceList::deviceChanged, this, &Audio::onInputChanged);

    enableNoiseReduction(enableNoiseReductionSetting.get());
}

void Audio::setReverb(bool enable) {
    DependencyManager::get<AudioClient>()->setReverb(enable);
}

void Audio::setReverbOptions(const AudioEffectOptions* options) {
    DependencyManager::get<AudioClient>()->setReverbOptions(options);
}

void Audio::onChangedContext() {
    bool isHMD = qApp->isHMDMode();
    if (_contextIsHMD != isHMD) {
        _contextIsHMD = isHMD;
        _devices.restoreDevices(_contextIsHMD);
        emit changedContext(getContext());
    }
}

QString Audio::getContext() const {
     return _contextIsHMD ? Audio::HMD : Audio::DESKTOP;
}

void Audio::onChangedMuted() {
    auto client = DependencyManager::get<AudioClient>().data();
    bool isMuted;
    QMetaObject::invokeMethod(client, "isMuted", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isMuted));

    if (_isMuted != isMuted) {
        _isMuted = isMuted;
        emit changedMuted(_isMuted);
    }
}

void Audio::setMuted(bool isMuted) {
    if (_isMuted != isMuted) {
        auto client = DependencyManager::get<AudioClient>().data();
        QMetaObject::invokeMethod(client, "toggleMute", Qt::BlockingQueuedConnection);

        _isMuted = isMuted;
        emit changedMuted(_isMuted);
    }
}

void Audio::enableNoiseReduction(bool enable) {
    if (_enableNoiseReduction != enable) {
        auto client = DependencyManager::get<AudioClient>().data();
        QMetaObject::invokeMethod(client, "setNoiseReduction", Qt::BlockingQueuedConnection, Q_ARG(bool, enable));

        enableNoiseReductionSetting.set(enable);
        _enableNoiseReduction = enable;
        emit changedNoiseReduction(enable);
    }
}

void Audio::setInputVolume(float volume) {
    // getInputVolume will not reflect changes synchronously, so clamp beforehand
    volume = glm::clamp(volume, 0.0f, 1.0f);

    if (_inputVolume != volume) {
        auto client = DependencyManager::get<AudioClient>().data();
        QMetaObject::invokeMethod(client, "setInputVolume", Qt::BlockingQueuedConnection, Q_ARG(float, volume));

        _inputVolume = volume;
        emit changedInputVolume(_inputVolume);
    }
}

void Audio::onInputChanged() {
    auto client = DependencyManager::get<AudioClient>().data();
    float volume;
    QMetaObject::invokeMethod(client, "getInputVolume", Qt::BlockingQueuedConnection, Q_RETURN_ARG(float, volume));

    if (_inputVolume != volume) {
        _inputVolume = volume;
        emit changedInputVolume(_inputVolume);
    }
}