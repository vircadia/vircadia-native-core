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
#include "AudioHelpers.h"
#include "ui/AvatarInputs.h"

using namespace scripting;

QString Audio::AUDIO { "Audio" };
QString Audio::DESKTOP { "Desktop" };
QString Audio::HMD { "VR" };

Setting::Handle<bool> enableNoiseReductionSetting { QStringList { Audio::AUDIO, "NoiseReduction" }, true };
Setting::Handle<bool> enableWarnWhenMutedSetting { QStringList { Audio::AUDIO, "WarnWhenMuted" }, true };


float Audio::loudnessToLevel(float loudness) {
    float level = loudness * (1/32768.0f);  // level in [0, 1]
    level = 6.02059991f * fastLog2f(level); // convert to dBFS
    level = (level + 48.0f) * (1/42.0f);    // map [-48, -6] dBFS to [0, 1]
    return glm::clamp(level, 0.0f, 1.0f);
}

Audio::Audio() : _devices(_contextIsHMD) {
    auto client = DependencyManager::get<AudioClient>().data();
    connect(client, &AudioClient::muteToggled, this, &Audio::setMuted);
    connect(client, &AudioClient::noiseReductionChanged, this, &Audio::enableNoiseReduction);
    connect(client, &AudioClient::warnWhenMutedChanged, this, &Audio::enableWarnWhenMuted);
    connect(client, &AudioClient::inputLoudnessChanged, this, &Audio::onInputLoudnessChanged);
    connect(client, &AudioClient::inputVolumeChanged, this, &Audio::setInputVolume);
    connect(this, &Audio::contextChanged, &_devices, &AudioDevices::onContextChanged);
    connect(this, &Audio::pushingToTalkChanged, this, &Audio::handlePushedToTalk);
    enableNoiseReduction(enableNoiseReductionSetting.get());
    enableWarnWhenMuted(enableWarnWhenMutedSetting.get());
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
    bool isHMD = qApp->isHMDMode();
    if (isHMD) {
        return getMutedHMD();
    } else {
        return getMutedDesktop();
    }
}

void Audio::setMuted(bool isMuted) {
    bool isHMD = qApp->isHMDMode();
    if (isHMD) {
        setMutedHMD(isMuted);
    } else {
        setMutedDesktop(isMuted);
    }
}

void Audio::setMutedDesktop(bool isMuted) {
    bool changed = false;
    withWriteLock([&] {
        if (_mutedDesktop != isMuted) {
            changed = true;
            _mutedDesktop = isMuted;
            auto client = DependencyManager::get<AudioClient>().data();
            QMetaObject::invokeMethod(client, "setMuted", Q_ARG(bool, isMuted), Q_ARG(bool, false));
        }
    });
    if (changed) {
        emit mutedChanged(isMuted);
        emit mutedDesktopChanged(isMuted);
    }
}

bool Audio::getMutedDesktop() const {
    return resultWithReadLock<bool>([&] {
        return _mutedDesktop;
    });
}

void Audio::setMutedHMD(bool isMuted) {
    bool changed = false;
    withWriteLock([&] {
        if (_mutedHMD != isMuted) {
            changed = true;
            _mutedHMD = isMuted;
            auto client = DependencyManager::get<AudioClient>().data();
            QMetaObject::invokeMethod(client, "setMuted", Q_ARG(bool, isMuted), Q_ARG(bool, false));
        }
    });
    if (changed) {
        emit mutedChanged(isMuted);
        emit mutedHMDChanged(isMuted);
    }
}

bool Audio::getMutedHMD() const {
    return resultWithReadLock<bool>([&] {
        return _mutedHMD;
    });
}

bool Audio::getPTT() {
    bool isHMD = qApp->isHMDMode();
    if (isHMD) {
        return getPTTHMD();
    } else {
        return getPTTDesktop();
    }
}

void scripting::Audio::setPushingToTalk(bool pushingToTalk) {
    bool changed = false;
    withWriteLock([&] {
        if (_pushingToTalk != pushingToTalk) {
            changed = true;
            _pushingToTalk = pushingToTalk;
        }
    });
    if (changed) {
        emit pushingToTalkChanged(pushingToTalk);
    }
}

bool Audio::getPushingToTalk() const {
    return resultWithReadLock<bool>([&] {
        return _pushingToTalk;
    });
}

void Audio::setPTT(bool enabled) {
    bool isHMD = qApp->isHMDMode();
    if (isHMD) {
        setPTTHMD(enabled);
    } else {
        setPTTDesktop(enabled);
    }
}

void Audio::setPTTDesktop(bool enabled) {
    bool changed = false;
    withWriteLock([&] {
        if (_pttDesktop != enabled) {
            changed = true;
            _pttDesktop = enabled;
        }
    });
    if (!enabled && _settingsLoaded) {
        // Set to default behavior (unmuted for Desktop) on Push-To-Talk disable.
        setMutedDesktop(true);
    } else {
        // Should be muted when not pushing to talk while PTT is enabled.
        setMutedDesktop(true);
    }

    if (changed) {
        emit pushToTalkChanged(enabled);
        emit pushToTalkDesktopChanged(enabled);
    }
}

bool Audio::getPTTDesktop() const {
    return resultWithReadLock<bool>([&] {
        return _pttDesktop;
    });
}

void Audio::setPTTHMD(bool enabled) {
    bool changed = false;
    withWriteLock([&] {
        if (_pttHMD != enabled) {
            changed = true;
            _pttHMD = enabled;
        }
    });
    if (!enabled && _settingsLoaded) {
        // Set to default behavior (unmuted for HMD) on Push-To-Talk disable.
        setMutedHMD(false);
    } else {
        // Should be muted when not pushing to talk while PTT is enabled.
        setMutedHMD(true);
    }

    if (changed) {
        emit pushToTalkChanged(enabled);
        emit pushToTalkHMDChanged(enabled);
    }
}

void Audio::saveData() {
    _mutedDesktopSetting.set(getMutedDesktop());
    _mutedHMDSetting.set(getMutedHMD());
    _pttDesktopSetting.set(getPTTDesktop());
    _pttHMDSetting.set(getPTTHMD());
}

void Audio::loadData() {
    setMutedDesktop(_mutedDesktopSetting.get());
    setMutedHMD(_mutedHMDSetting.get());
    setPTTDesktop(_pttDesktopSetting.get());
    setPTTHMD(_pttHMDSetting.get());

    auto client = DependencyManager::get<AudioClient>().data();
    QMetaObject::invokeMethod(client, "setMuted", Q_ARG(bool, isMuted()), Q_ARG(bool, false));
    _settingsLoaded = true;
}

bool Audio::getPTTHMD() const {
    return resultWithReadLock<bool>([&] {
        return _pttHMD;
    });
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

bool Audio::warnWhenMutedEnabled() const {
    return resultWithReadLock<bool>([&] {
        return _enableWarnWhenMuted;
    });
}

void Audio::enableWarnWhenMuted(bool enable) {
    bool changed = false;
    withWriteLock([&] {
        if (_enableWarnWhenMuted != enable) {
            _enableWarnWhenMuted = enable;
            auto client = DependencyManager::get<AudioClient>().data();
            QMetaObject::invokeMethod(client, "setWarnWhenMuted", Q_ARG(bool, enable), Q_ARG(bool, false));
            enableWarnWhenMutedSetting.set(enable);
            changed = true;
        }
    });
    if (changed) {
        emit warnWhenMutedChanged(enable);
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

bool Audio::isClipping() const {
    return resultWithReadLock<bool>([&] {
        return _isClipping;
    });
}

void Audio::onInputLoudnessChanged(float loudness, bool isClipping) {
    float level = loudnessToLevel(loudness);
    bool levelChanged = false;
    bool isClippingChanged = false;

    withWriteLock([&] {
        if (_inputLevel != level) {
            _inputLevel = level;
            levelChanged = true;
        }
        if (_isClipping != isClipping) {
            _isClipping = isClipping;
            isClippingChanged = true;
        }
    });
    if (levelChanged) {
        emit inputLevelChanged(level);
    }
    if (isClippingChanged) {
        emit clippingChanged(isClipping);
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
    if (isHMD) {
        setMuted(getMutedHMD());
    } else {
        setMuted(getMutedDesktop());
    }
    if (changed) {
        emit contextChanged(isHMD ? Audio::HMD : Audio::DESKTOP);
    }
}

void Audio::handlePushedToTalk(bool enabled) {
    if (getPTT()) {
        if (enabled) {
            setMuted(false);
        } else {
            setMuted(true);
        }
    }
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

void Audio::setAvatarGain(float gain) {
    withWriteLock([&] {
        // ask the NodeList to set the master avatar gain
        DependencyManager::get<NodeList>()->setAvatarGain(QUuid(), gain);
    });
}

float Audio::getAvatarGain() {
    return resultWithReadLock<float>([&] {
        return DependencyManager::get<NodeList>()->getAvatarGain(QUuid());
    });
}

void Audio::setInjectorGain(float gain) {
    withWriteLock([&] {
        // ask the NodeList to set the audio injector gain
        DependencyManager::get<NodeList>()->setInjectorGain(gain);
    });
}

float Audio::getInjectorGain() {
    return resultWithReadLock<float>([&] {
        return DependencyManager::get<NodeList>()->getInjectorGain();
    });
}

void Audio::setLocalInjectorGain(float gain) {
    withWriteLock([&] {
        if (_localInjectorGain != gain) {
            _localInjectorGain = gain;
            // convert dB to amplitude
            gain = fastExp2f(gain / 6.02059991f);
            // quantize and limit to match NodeList::setInjectorGain()
            gain = unpackFloatGainFromByte(packFloatGainToByte(gain));
            DependencyManager::get<AudioClient>()->setLocalInjectorGain(gain);
        }
    });
}

float Audio::getLocalInjectorGain() {
    return resultWithReadLock<float>([&] {
        return _localInjectorGain;
    });
}

void Audio::setSystemInjectorGain(float gain) {
    withWriteLock([&] {
        if (_systemInjectorGain != gain) {
            _systemInjectorGain = gain;
            // convert dB to amplitude
            gain = fastExp2f(gain / 6.02059991f);
            // quantize and limit to match NodeList::setInjectorGain()
            gain = unpackFloatGainFromByte(packFloatGainToByte(gain));
            DependencyManager::get<AudioClient>()->setSystemInjectorGain(gain);
        }
    });
}

float Audio::getSystemInjectorGain() {
    return resultWithReadLock<float>([&] {
        return _systemInjectorGain;
    });
}
