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
    connect(client, &AudioClient::muteToggled, this, &Audio::setMuted);
    connect(client, &AudioClient::noiseReductionChanged, this, &Audio::enableNoiseReduction);
    connect(client, &AudioClient::inputLoudnessChanged, this, &Audio::onInputLoudnessChanged);
    connect(client, &AudioClient::inputVolumeChanged, this, &Audio::setInputVolume);
    connect(this, &Audio::contextChanged, &_devices, &AudioDevices::onContextChanged);
    enableNoiseReduction(enableNoiseReductionSetting.get());
    onContextChanged();
}

bool Audio::startRecording(const QString& filepath) {
    return resultWithWriteLock<bool>([&] {
        return DependencyManager::get<AudioClient>()->startRecording(filepath);
    });
}

bool Audio::getRecording() {
    return resultWithReadLock<bool>([&] {
        return DependencyManager::get<AudioClient>()->getRecording();
    });
}

void Audio::stopRecording() {
    withWriteLock([&] {
        DependencyManager::get<AudioClient>()->stopRecording();
    });
}

bool Audio::isMuted() const {
    return resultWithReadLock<bool>([&] {
        return _isMuted;
    });
}

void Audio::setMuted(bool isMuted) {
    bool changed = false;
    withWriteLock([&] {
        if (_isMuted != isMuted) {
            _isMuted = isMuted;
            auto client = DependencyManager::get<AudioClient>().data();
            QMetaObject::invokeMethod(client, "setMuted", Q_ARG(bool, isMuted), Q_ARG(bool, false));
            changed = true;
        }
    });
    if (changed) {
        emit mutedChanged(isMuted);
    }
}

bool Audio::noiseReductionEnabled() const {
    return resultWithReadLock<bool>([&] {
        return _enableNoiseReduction;
    });
}

void Audio::enableNoiseReduction(bool enable) {
    bool changed = false;
    withWriteLock([&] {
        if (_enableNoiseReduction != enable) {
            _enableNoiseReduction = enable;
            auto client = DependencyManager::get<AudioClient>().data();
            QMetaObject::invokeMethod(client, "setNoiseReduction", Q_ARG(bool, enable), Q_ARG(bool, false));
            enableNoiseReductionSetting.set(enable);
            changed = true;
        }
    });
    if (changed) {
        emit noiseReductionChanged(enable);
    }
}

float Audio::getInputVolume() const {
    return resultWithReadLock<bool>([&] {
        return _inputVolume;
    });
}

void Audio::setInputVolume(float volume) {
    // getInputVolume will not reflect changes synchronously, so clamp beforehand
    volume = glm::clamp(volume, 0.0f, 1.0f);

    bool changed = false;
    withWriteLock([&] {
        if (_inputVolume != volume) {
            _inputVolume = volume;
            auto client = DependencyManager::get<AudioClient>().data();
            QMetaObject::invokeMethod(client, "setInputVolume", Q_ARG(float, volume), Q_ARG(bool, false));
            changed = true;
        }
    });
    if (changed) {
        emit inputVolumeChanged(volume);
    }
}

float Audio::getInputLevel() const {
    return resultWithReadLock<float>([&] {
        return _inputLevel;
    });
}

void Audio::onInputLoudnessChanged(float loudness) {
    float level = loudnessToLevel(loudness);
    bool changed = false;
    withWriteLock([&] {
        if (_inputLevel != level) {
            _inputLevel = level;
            changed = true;
        }
    });
    if (changed) {
        emit inputLevelChanged(level);
    }
}

QString Audio::getContext() const {
    return resultWithReadLock<QString>([&] {
        return _contextIsHMD ? Audio::HMD : Audio::DESKTOP;
    });
}

void Audio::onContextChanged() {
    bool changed = false;
    bool isHMD = qApp->isHMDMode();
    withWriteLock([&] {
        if (_contextIsHMD != isHMD) {
            _contextIsHMD = isHMD;
            changed = true;
        }
    });
    if (changed) {
        emit contextChanged(isHMD ? Audio::HMD : Audio::DESKTOP);
    }
}

void Audio::setReverb(bool enable) {
    withWriteLock([&] {
        DependencyManager::get<AudioClient>()->setReverb(enable);
    });
}

void Audio::setReverbOptions(const AudioEffectOptions* options) {
    withWriteLock([&] {
        DependencyManager::get<AudioClient>()->setReverbOptions(options);
    });
}

void Audio::setInputDevice(const QAudioDeviceInfo& device, bool isHMD) {
    withWriteLock([&] {
        _devices.chooseInputDevice(device, isHMD);
    });
}

void Audio::setOutputDevice(const QAudioDeviceInfo& device, bool isHMD) {
    withWriteLock([&] {
        _devices.chooseOutputDevice(device, isHMD);
    });
}
