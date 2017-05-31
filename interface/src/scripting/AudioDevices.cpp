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
#include "AudioClient.h"

using namespace scripting;

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
                emit dataChanged(index, index, { Qt::CheckStateRole });
            }
        }
    }

    return success;
}

void AudioDeviceList::setDevice(const QAudioDeviceInfo& device) {
    _selectedDevice = device;

    for (auto i = 0; i < _devices.size(); ++i) {
        AudioDevice& device = _devices[i];

        if (device.selected && device.info != _selectedDevice) {
            device.selected = false;
            auto index = createIndex(i , 0);
            emit dataChanged(index, index, { Qt::CheckStateRole });
        } else if (!device.selected && device.info == _selectedDevice) {
            device.selected = true;
            auto index = createIndex(i , 0);
            emit dataChanged(index, index, { Qt::CheckStateRole });
        }
    }
}

void AudioDeviceList::populate(const QList<QAudioDeviceInfo>& devices) {
    beginResetModel();

    _devices.clear();
    foreach(const QAudioDeviceInfo& deviceInfo, devices) {
        AudioDevice device;
        device.info = deviceInfo;
        device.name = device.info.deviceName();
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

void AudioDevices::onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device) {
    if (mode == QAudio::AudioInput) {
        _inputs.setDevice(device);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.setDevice(device);
    }
}

void AudioDevices::onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices) {
    if (mode == QAudio::AudioInput) {
        _inputs.populate(devices);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.populate(devices);
    }
}