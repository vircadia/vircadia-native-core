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

QStringList AudioDeviceScriptingInterface::inputAudioDevices() const
{
    return DependencyManager::get<AudioClient>()->getDeviceNames(QAudio::AudioInput).toList();;
}

QStringList AudioDeviceScriptingInterface::outputAudioDevices() const
{
    return DependencyManager::get<AudioClient>()->getDeviceNames(QAudio::AudioOutput).toList();;
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

void AudioDeviceScriptingInterface::setInputDeviceAsync(const QString& deviceName) {
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "switchInputToAudioDevice",
                              Qt::QueuedConnection,
                              Q_ARG(const QString&, deviceName));
}

void AudioDeviceScriptingInterface::setOutputDeviceAsync(const QString& deviceName) {
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
    _devices.clear();
    const QString &outDevice = getOutputDevice();
    for (QString name: getOutputDevices()) {
        AudioDeviceInfo di;
        di.name = name;
        di.selected = (name == outDevice);
        di.mode = QAudio::AudioOutput;
        _devices.append(di);
    }
    const QString &inDevice = getInputDevice();
    for (QString name: getInputDevices()) {
        AudioDeviceInfo di;
        di.name = name;
        di.selected = (name == inDevice);
        di.mode = QAudio::AudioInput;
        _devices.append(di);
    }

    endResetModel();
    emit deviceChanged();
}

void AudioDeviceScriptingInterface::onCurrentInputDeviceChanged()
{
    currentDeviceUpdate(getInputDevice(), QAudio::AudioInput);
    emit currentInputDeviceChanged();
}

void AudioDeviceScriptingInterface::onCurrentOutputDeviceChanged()
{
    currentDeviceUpdate(getOutputDevice(), QAudio::AudioOutput);
    emit currentOutputDeviceChanged();
}

void AudioDeviceScriptingInterface::currentDeviceUpdate(const QString &name, QAudio::Mode mode)
{
    QVector<int> role;
    role.append(SelectedRole);

    for (int i = 0; i < _devices.size(); i++) {
        AudioDeviceInfo di = _devices.at(i);
        if (di.mode != mode)
            continue;
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
