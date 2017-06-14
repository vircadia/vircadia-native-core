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

Setting::Handle<QString> inputDeviceDesktop { QStringList { Audio::AUDIO, Audio::DESKTOP, "INPUT" }};
Setting::Handle<QString> outputDeviceDesktop { QStringList { Audio::AUDIO, Audio::DESKTOP, "OUTPUT" }};
Setting::Handle<QString> inputDeviceHMD { QStringList { Audio::AUDIO, Audio::HMD, "INPUT" }};
Setting::Handle<QString> outputDeviceHMD { QStringList { Audio::AUDIO, Audio::HMD, "OUTPUT" }};

QHash<int, QByteArray> AudioDeviceList::_roles {
    { Qt::DisplayRole, "display" },
    { Qt::CheckStateRole, "selected" }
};
Qt::ItemFlags AudioDeviceList::_flags { Qt::ItemIsSelectable | Qt::ItemIsEnabled };

QVariant AudioDeviceList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= _devices.size()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return _devices.at(index.row()).display;
    } else if (role == Qt::CheckStateRole) {
        return _devices.at(index.row()).selected;
    } else {
        return QVariant();
    }
}

bool AudioDeviceList::setData(const QModelIndex& index, const QVariant& value, int role) {
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
            QMetaObject::invokeMethod(client.data(), "switchAudioDevice", Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, success),
                Q_ARG(QAudio::Mode, _mode),
                Q_ARG(const QAudioDeviceInfo&, device.info));

            if (success) {
                device.selected = true;
                emit deviceSelected(device.info);
                emit deviceChanged(device.info);
            }
        }
    }

    emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
    return success;
}

void AudioDeviceList::resetDevice(bool contextIsHMD, const QString& device) {
    bool success { false };

    // try to set the last selected device
    if (!device.isNull()) {
        auto i = 0;
        for (; i < rowCount(); ++i) {
            if (device == _devices[i].info.deviceName()) {
                break;
            }
        }
        if (i < rowCount()) {
            success = setData(createIndex(i, 0), true, Qt::CheckStateRole);
        }

        // the selection failed - reset it
        if (!success) {
            emit deviceSelected(QAudioDeviceInfo());
        }
    }

    // try to set to the default device for this mode
    if (!success) {
        auto client = DependencyManager::get<AudioClient>().data();
        if (contextIsHMD) {
            QString deviceName;
            if (_mode == QAudio::AudioInput) {
                deviceName = qApp->getActiveDisplayPlugin()->getPreferredAudioInDevice();
            } else { // if (_mode == QAudio::AudioOutput)
                deviceName = qApp->getActiveDisplayPlugin()->getPreferredAudioOutDevice();
            }
            if (!deviceName.isNull()) {
                QMetaObject::invokeMethod(client, "switchAudioDevice", Q_ARG(QAudio::Mode, _mode), Q_ARG(QString, deviceName));
            }
        } else {
            // use the system default
            QMetaObject::invokeMethod(client, "switchAudioDevice", Q_ARG(QAudio::Mode, _mode));
        }
    }
}

void AudioDeviceList::onDeviceChanged(const QAudioDeviceInfo& device) {
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

    emit deviceChanged(_selectedDevice);
    emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

void AudioDeviceList::onDevicesChanged(const QList<QAudioDeviceInfo>& devices) {
    beginResetModel();

    _devices.clear();

    foreach(const QAudioDeviceInfo& deviceInfo, devices) {
        AudioDevice device;
        device.info = deviceInfo;
        device.display = device.info.deviceName()
            .replace("High Definition", "HD")
            .remove("Device")
            .replace(" )", ")");
        device.selected = (device.info == _selectedDevice);
        _devices.push_back(device);
    }

    endResetModel();
}

AudioDevices::AudioDevices(bool& contextIsHMD) : _contextIsHMD(contextIsHMD) {
    auto client = DependencyManager::get<AudioClient>();

    connect(client.data(), &AudioClient::deviceChanged, this, &AudioDevices::onDeviceChanged, Qt::QueuedConnection);
    connect(client.data(), &AudioClient::devicesChanged, this, &AudioDevices::onDevicesChanged, Qt::QueuedConnection);

    // connections are made after client is initialized, so we must also fetch the devices
    _inputs.onDeviceChanged(client->getActiveAudioDevice(QAudio::AudioInput));
    _outputs.onDeviceChanged(client->getActiveAudioDevice(QAudio::AudioOutput));
    _inputs.onDevicesChanged(client->getAudioDevices(QAudio::AudioInput));
    _outputs.onDevicesChanged(client->getAudioDevices(QAudio::AudioOutput));

    connect(&_inputs, &AudioDeviceList::deviceSelected, this, &AudioDevices::onInputDeviceSelected);
    connect(&_outputs, &AudioDeviceList::deviceSelected, this, &AudioDevices::onOutputDeviceSelected);
}

void AudioDevices::onContextChanged(const QString& context) {
    QString input;
    QString output;
    if (_contextIsHMD) {
        input = inputDeviceHMD.get();
        output = outputDeviceHMD.get();
    } else {
        input = inputDeviceDesktop.get();
        output = outputDeviceDesktop.get();
    }

    _inputs.resetDevice(_contextIsHMD, input);
    _outputs.resetDevice(_contextIsHMD, output);
}

void AudioDevices::onInputDeviceSelected(const QAudioDeviceInfo& device) {
    QString deviceName;
    if (!device.isNull()) {
        deviceName = device.deviceName();
    }

    if (_contextIsHMD) {
        inputDeviceHMD.set(deviceName);
    } else {
        inputDeviceDesktop.set(deviceName);
    }
}

void AudioDevices::onOutputDeviceSelected(const QAudioDeviceInfo& device) {
    QString deviceName;
    if (!device.isNull()) {
        deviceName = device.deviceName();
    }

    if (_contextIsHMD) {
        outputDeviceHMD.set(deviceName);
    } else {
        outputDeviceDesktop.set(deviceName);
    }
}

void AudioDevices::onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device) {
    if (mode == QAudio::AudioInput) {
        _inputs.onDeviceChanged(device);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.onDeviceChanged(device);
    }
}

void AudioDevices::onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices) {
    static bool initialized { false };
    auto initialize = [&]{
        if (initialized) {
            onContextChanged(QString());
        } else {
            initialized = true;
        }
    };

    if (mode == QAudio::AudioInput) {
        _inputs.onDevicesChanged(devices);
        static std::once_flag inputFlag;
        std::call_once(inputFlag, initialize);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.onDevicesChanged(devices);
        static std::once_flag outputFlag;
        std::call_once(outputFlag, initialize);
    }
}