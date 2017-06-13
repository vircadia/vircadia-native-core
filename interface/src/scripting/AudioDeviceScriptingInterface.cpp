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

#include <AudioClient.h>
#include <AudioClientLogging.h>

#include "AudioDeviceScriptingInterface.h"
#include "SettingsScriptingInterface.h"

AudioDeviceScriptingInterface* AudioDeviceScriptingInterface::getInstance() {
    static AudioDeviceScriptingInterface sharedInstance;
    return &sharedInstance;
}

QStringList AudioDeviceScriptingInterface::inputAudioDevices() const {
    return _inputAudioDevices;
}

QStringList AudioDeviceScriptingInterface::outputAudioDevices() const {
    return _outputAudioDevices;
}

bool AudioDeviceScriptingInterface::muted()
{
    return getMuted();
}

AudioDeviceScriptingInterface::AudioDeviceScriptingInterface(): QAbstractListModel(nullptr) {
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::muteToggled,
            this, &AudioDeviceScriptingInterface::muteToggled);
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::deviceChanged,
        this, &AudioDeviceScriptingInterface::onDeviceChanged, Qt::QueuedConnection);
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::currentInputDeviceChanged,
        this, &AudioDeviceScriptingInterface::onCurrentInputDeviceChanged, Qt::QueuedConnection);
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::currentOutputDeviceChanged,
        this, &AudioDeviceScriptingInterface::onCurrentOutputDeviceChanged, Qt::QueuedConnection);
    //fill up model
    onDeviceChanged();
    //set up previously saved device
    SettingsScriptingInterface* settings = SettingsScriptingInterface::getInstance();
    const QString inDevice = settings->getValue("audio_input_device", _currentInputDevice).toString();
    if (inDevice != _currentInputDevice) {
        qCDebug(audioclient) << __FUNCTION__ << "about to call setInputDeviceAsync() device: [" << inDevice << "] _currentInputDevice:" << _currentInputDevice;
        setInputDeviceAsync(inDevice);
    }

    // If the audio_output_device setting is not available, use the _currentOutputDevice
    auto outDevice = settings->getValue("audio_output_device", _currentOutputDevice).toString();
    if (outDevice != _currentOutputDevice) {
        qCDebug(audioclient) << __FUNCTION__ << "about to call setOutputDeviceAsync() outDevice: [" << outDevice << "] _currentOutputDevice:" << _currentOutputDevice;
        setOutputDeviceAsync(outDevice);
    }
}

bool AudioDeviceScriptingInterface::setInputDevice(const QString& deviceName) {
    qCDebug(audioclient) << __FUNCTION__ << "deviceName:" << deviceName;

    bool result;
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchInputToAudioDevice",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(const QString&, deviceName));
    return result;
}

bool AudioDeviceScriptingInterface::setOutputDevice(const QString& deviceName) {

    qCDebug(audioclient) << __FUNCTION__ << "deviceName:" << deviceName;

    bool result;
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchOutputToAudioDevice",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(const QString&, deviceName));
    return result;
}

bool AudioDeviceScriptingInterface::setDeviceFromMenu(const QString& deviceMenuName) {
    QAudio::Mode mode;

    if (deviceMenuName.indexOf("for Output") != -1) {
        mode = QAudio::AudioOutput;
    } else if (deviceMenuName.indexOf("for Input") != -1) {
        mode = QAudio::AudioInput;
    } else {
        return false;
    }

    for (ScriptingAudioDeviceInfo di: _devices) {
        if (mode == di.mode && deviceMenuName.contains(di.name)) {
            if (mode == QAudio::AudioOutput) {
                qCDebug(audioclient) << __FUNCTION__ << "about to call setOutputDeviceAsync() device: [" << di.name << "]";
                setOutputDeviceAsync(di.name);
            } else {
                qCDebug(audioclient) << __FUNCTION__ << "about to call setInputDeviceAsync() device: [" << di.name << "]";
                setInputDeviceAsync(di.name);
            }
            return true;
        }
    }

    return false;
}

void AudioDeviceScriptingInterface::setInputDeviceAsync(const QString& deviceName) {
    qCDebug(audioclient) << __FUNCTION__ << "deviceName:" << deviceName;

    if (deviceName.isEmpty()) {
        qCDebug(audioclient) << __FUNCTION__ << "attempt to set empty deviceName:" << deviceName << "... ignoring!";
        return;
    }

    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchInputToAudioDevice",
                              Qt::QueuedConnection,
                              Q_ARG(const QString&, deviceName));
}

void AudioDeviceScriptingInterface::setOutputDeviceAsync(const QString& deviceName) {
    qCDebug(audioclient) << __FUNCTION__ << "deviceName:" << deviceName;

    if (deviceName.isEmpty()) {
        qCDebug(audioclient) << __FUNCTION__ << "attempt to set empty deviceName:" << deviceName << "... ignoring!";
        return;
    }

    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchOutputToAudioDevice",
                              Qt::QueuedConnection,
                              Q_ARG(const QString&, deviceName));
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

void AudioDeviceScriptingInterface::setMuted(bool muted)
{
    bool lMuted = getMuted();
    if (lMuted == muted)
        return;

    toggleMute();
    lMuted = getMuted();
    emit mutedChanged(lMuted);
}

bool AudioDeviceScriptingInterface::getMuted() {
    return DependencyManager::get<AudioClient>()->isMuted();
}

QVariant AudioDeviceScriptingInterface::data(const QModelIndex& index, int role) const {
    //sanity
    if (!index.isValid() || index.row() >= _devices.size())
        return QVariant();


    if (role == Qt::DisplayRole || role == DisplayNameRole) {
        return _devices.at(index.row()).name;
    } else if (role == SelectedRole) {
        return _devices.at(index.row()).selected;
    } else if (role == AudioModeRole) {
        return (int)_devices.at(index.row()).mode;
    }
    return QVariant();
}

int AudioDeviceScriptingInterface::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return _devices.size();
}

QHash<int, QByteArray> AudioDeviceScriptingInterface::roleNames() const {
    QHash<int, QByteArray> roles;
    roles.insert(DisplayNameRole, "devicename");
    roles.insert(SelectedRole, "devicechecked");
    roles.insert(AudioModeRole, "devicemode");
    return roles;
}

void AudioDeviceScriptingInterface::onDeviceChanged()
{
    beginResetModel();
    _outputAudioDevices.clear();
    _devices.clear();
    _currentOutputDevice = getOutputDevice();
    for (QString name: getOutputDevices()) {
        ScriptingAudioDeviceInfo di;
        di.name = name;
        di.selected = (name == _currentOutputDevice);
        di.mode = QAudio::AudioOutput;
        _devices.append(di);
        _outputAudioDevices.append(name);
    }
    emit outputAudioDevicesChanged(_outputAudioDevices);

    _inputAudioDevices.clear();
    _currentInputDevice = getInputDevice();
    for (QString name: getInputDevices()) {
        ScriptingAudioDeviceInfo di;
        di.name = name;
        di.selected = (name == _currentInputDevice);
        di.mode = QAudio::AudioInput;
        _devices.append(di);
        _inputAudioDevices.append(name);
    }
    emit inputAudioDevicesChanged(_inputAudioDevices);

    endResetModel();
    emit deviceChanged();
}

void AudioDeviceScriptingInterface::onCurrentInputDeviceChanged(const QString& name)
{
    currentDeviceUpdate(name, QAudio::AudioInput);
    //we got a signal that device changed. Save it now
    SettingsScriptingInterface* settings = SettingsScriptingInterface::getInstance();
    settings->setValue("audio_input_device", name);
    emit currentInputDeviceChanged(name);
}

void AudioDeviceScriptingInterface::onCurrentOutputDeviceChanged(const QString& name)
{
    currentDeviceUpdate(name, QAudio::AudioOutput);

    // FIXME - this is kinda janky to set the setting on the result of a signal
    //we got a signal that device changed. Save it now
    SettingsScriptingInterface* settings = SettingsScriptingInterface::getInstance();
    qCDebug(audioclient) << __FUNCTION__ << "about to call settings->setValue('audio_output_device', name); name:" << name;
    settings->setValue("audio_output_device", name);
    emit currentOutputDeviceChanged(name);
}

void AudioDeviceScriptingInterface::currentDeviceUpdate(const QString& name, QAudio::Mode mode)
{
    QVector<int> role;
    role.append(SelectedRole);

    for (int i = 0; i < _devices.size(); i++) {
        ScriptingAudioDeviceInfo di = _devices.at(i);
        if (di.mode != mode) {
            continue;
        }
        if (di.selected && di.name != name ) {
            di.selected = false;
            _devices[i] = di;
            emit dataChanged(index(i, 0), index(i, 0), role);
        }
        if (di.name == name) {
            di.selected = true;
            _devices[i] = di;
            emit dataChanged(index(i, 0), index(i, 0), role);
        }
    }
}
