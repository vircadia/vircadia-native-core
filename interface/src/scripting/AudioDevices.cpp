//
//  AudioDevices.cpp
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 28/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioDevices.h"

#include "Application.h"
#include "AudioClient.h"
#include "Audio.h"

using namespace scripting;

const QString INPUT { "INPUT" };
const QString OUTPUT { "OUTPUT" };

Setting::Handle<QAudioDeviceInfo> desktopInputDevice { QStringList { Audio::AUDIO, Audio::DESKTOP, INPUT }};
Setting::Handle<QAudioDeviceInfo> desktopOutputDevice { QStringList { Audio::AUDIO, Audio::DESKTOP, OUTPUT }};
Setting::Handle<QAudioDeviceInfo> HMDInputDevice { QStringList { Audio::AUDIO, Audio::HMD, INPUT }};
Setting::Handle<QAudioDeviceInfo> HMDOutputDevice { QStringList { Audio::AUDIO, Audio::HMD, OUTPUT }};

QHash<int, QByteArray> AudioDeviceList::_roles {
    { Qt::DisplayRole, "display" },
    { Qt::CheckStateRole, "selected" }
};
Qt::ItemFlags AudioDeviceList::_flags { Qt::ItemIsSelectable | Qt::ItemIsEnabled };

AudioDeviceList::AudioDeviceList(QAudio::Mode mode) : _mode(mode) {
}

int AudioDeviceList::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return _devices.size();
}

QVariant AudioDeviceList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= _devices.size()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return _devices.at(index.row()).name;
    } else if (role == Qt::CheckStateRole) {
        return _devices.at(index.row()).selected;
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> AudioDeviceList::roleNames() const {
    return _roles;
}

Qt::ItemFlags AudioDeviceList::flags(const QModelIndex& index) const {
    return _flags;
}

bool AudioDeviceList::setData(const QModelIndex& index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() >= _devices.size()) {
        return false;
    }

    bool success = false;

    if (role == Qt::CheckStateRole) {
        auto selected = value.toBool();
        auto& device = _devices[index.row()];

            // only allow switching to a new device, not deactivating an in-use device
        if (selected
            // skip if already selected
            && selected != device.selected) {

            auto client = DependencyManager::get<AudioClient>();
            bool success;
            QMetaObject::invokeMethod(client.data(), "switchAudioDevice", Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, success),
                Q_ARG(QAudio::Mode, _mode),
                Q_ARG(const QAudioDeviceInfo&, device.info));

            if (success) {
                device.selected = true;
                emit deviceChanged();
            }
        }
    }

    emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
    return success;
}

void AudioDeviceList::setDevice(const QAudioDeviceInfo& device) {
    _selectedDevice = device;
    QModelIndex index;

    for (auto i = 0; i < _devices.size(); ++i) {
        AudioDevice& device = _devices[i];

        if (device.selected && device.info != _selectedDevice) {
            device.selected = false;
        } else if (device.info == _selectedDevice) {
            device.selected = true;
            index = createIndex(i, 0);
        }
    }

    emit deviceChanged();
    emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

void AudioDeviceList::populate(const QList<QAudioDeviceInfo>& devices) {
    beginResetModel();

    _devices.clear();
    _selectedDeviceIndex = QModelIndex();

    foreach(const QAudioDeviceInfo& deviceInfo, devices) {
        AudioDevice device;
        device.info = deviceInfo;
        device.name = device.info.deviceName()
            .replace("High Definition", "HD")
            .remove("Device");
        device.selected = (device.info == _selectedDevice);
        _devices.push_back(device);
    }

    endResetModel();
}

AudioDevices::AudioDevices() {
    auto client = DependencyManager::get<AudioClient>();
    // connections are made after client is initialized, so we must also fetch the devices

    connect(client.data(), &AudioClient::deviceChanged, this, &AudioDevices::onDeviceChanged, Qt::QueuedConnection);
    _inputs.setDevice(client->getActiveAudioDevice(QAudio::AudioInput));
    _outputs.setDevice(client->getActiveAudioDevice(QAudio::AudioOutput));

    connect(client.data(), &AudioClient::devicesChanged, this, &AudioDevices::onDevicesChanged, Qt::QueuedConnection);
    _inputs.populate(client->getAudioDevices(QAudio::AudioInput));
    _outputs.populate(client->getAudioDevices(QAudio::AudioOutput));
}

void AudioDevices::restoreDevices(bool isHMD) {
    // TODO
}

void AudioDevices::onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device) {
    if (mode == QAudio::AudioInput) {
        _inputs.setDevice(device);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.setDevice(device);
    }
}

void AudioDevices::onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices) {
    static bool initialized { false };
    auto initialize = [&]{
        if (initialized) {
            restoreDevices(qApp->isHMDMode());
        } else {
            initialized = true;
        }
    };

    if (mode == QAudio::AudioInput) {
        _inputs.populate(devices);
        static std::once_flag inputFlag;
        std::call_once(inputFlag, initialize);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.populate(devices);
        static std::once_flag outputFlag;
        std::call_once(outputFlag, initialize);
    }
}