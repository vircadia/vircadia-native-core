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

#include <shared/QtHelpers.h>

#include "Application.h"
#include "AudioClient.h"
#include "ui/AvatarInputs.h"

using namespace scripting;

QString Audio::AUDIO { "Audio" };
QString Audio::DESKTOP { "Desktop" };
QString Audio::HMD { "VR" };

Setting::Handle<bool> enableNoiseReductionSetting { QStringList { Audio::AUDIO, "NoiseReduction" }, true };

float Audio::loudnessToLevel(float loudness) {
    const float LOG2 = log(2.0f);
    const float METER_LOUDNESS_SCALE = 2.8f / 5.0f;
    const float LOG2_LOUDNESS_FLOOR = 11.0f;

    float level = 0.0f;

    loudness += 1.0f;
    float log2loudness = logf(loudness) / LOG2;

    if (log2loudness <= LOG2_LOUDNESS_FLOOR) {
        level = (log2loudness / LOG2_LOUDNESS_FLOOR) * METER_LOUDNESS_SCALE;
    } else {
        level = (log2loudness - (LOG2_LOUDNESS_FLOOR - 1.0f)) * METER_LOUDNESS_SCALE;
    }

    if (level > 1.0f) {
        level = 1.0;
    }

    return level;
}

Audio::Audio() : _devices(_contextIsHMD) {
    auto client = DependencyManager::get<AudioClient>().data();
    connect(client, &AudioClient::muteToggled, this, &Audio::onMutedChanged);
    connect(client, &AudioClient::noiseReductionChanged, this, &Audio::onNoiseReductionChanged);
    connect(client, &AudioClient::inputLoudnessChanged, this, &Audio::onInputLoudnessChanged);
    connect(client, &AudioClient::inputVolumeChanged, this, &Audio::onInputVolumeChanged);
    connect(this, &Audio::contextChanged, &_devices, &AudioDevices::onContextChanged);
    enableNoiseReduction(enableNoiseReductionSetting.get());
}

void Audio::setMuted(bool isMuted) {
    if (_isMuted != isMuted) {
        auto client = DependencyManager::get<AudioClient>().data();
        QMetaObject::invokeMethod(client, "toggleMute");
    }
}

void Audio::onMutedChanged() {
    bool isMuted = DependencyManager::get<AudioClient>()->isMuted();
    if (_isMuted != isMuted) {
        _isMuted = isMuted;
        emit mutedChanged(_isMuted);
    }
}

void Audio::enableNoiseReduction(bool enable) {
    if (_enableNoiseReduction != enable) {
        auto client = DependencyManager::get<AudioClient>().data();
        QMetaObject::invokeMethod(client, "setNoiseReduction", Q_ARG(bool, enable));
        enableNoiseReductionSetting.set(enable);
    }
}

void Audio::onNoiseReductionChanged() {
    bool noiseReductionEnabled = DependencyManager::get<AudioClient>()->isNoiseReductionEnabled();
    if (_enableNoiseReduction != noiseReductionEnabled) {
        _enableNoiseReduction = noiseReductionEnabled;
        emit noiseReductionChanged(_enableNoiseReduction);
    }
}

void Audio::setInputVolume(float volume) {
    // getInputVolume will not reflect changes synchronously, so clamp beforehand
    volume = glm::clamp(volume, 0.0f, 1.0f);

    if (_inputVolume != volume) {
        auto client = DependencyManager::get<AudioClient>().data();
        QMetaObject::invokeMethod(client, "setInputVolume", Q_ARG(float, volume));
    }
}

void Audio::onInputVolumeChanged(float volume) {
    if (_inputVolume != volume) {
        _inputVolume = volume;
        emit inputVolumeChanged(_inputVolume);
    }
}

void Audio::onInputLoudnessChanged(float loudness) {
    float level = loudnessToLevel(loudness);

    if (_inputLevel != level) {
        _inputLevel = level;
        emit inputLevelChanged(_inputLevel);
    }
}

QString Audio::getContext() const {
     return _contextIsHMD ? Audio::HMD : Audio::DESKTOP;
}

void Audio::onContextChanged() {
    bool isHMD = qApp->isHMDMode();
    if (_contextIsHMD != isHMD) {
        _contextIsHMD = isHMD;
        emit contextChanged(getContext());
    }
}

void Audio::setReverb(bool enable) {
    DependencyManager::get<AudioClient>()->setReverb(enable);
}

void Audio::setReverbOptions(const AudioEffectOptions* options) {
    DependencyManager::get<AudioClient>()->setReverbOptions(options);
}

void Audio::setInputDevice(const QAudioDeviceInfo& device) {
    _devices.chooseInputDevice(device);
}

void Audio::setOutputDevice(const QAudioDeviceInfo& device) {
    _devices.chooseOutputDevice(device);
}
