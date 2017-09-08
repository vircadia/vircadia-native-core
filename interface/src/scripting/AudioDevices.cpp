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

#include <map>

#include <shared/QtHelpers.h>
#include <plugins/DisplayPlugin.h>

#include "AudioDevices.h"

#include "Application.h"
#include "AudioClient.h"
#include "Audio.h"

#include "UserActivityLogger.h"

using namespace scripting;

static Setting::Handle<QString> desktopInputDeviceSetting { QStringList { Audio::AUDIO, Audio::DESKTOP, "INPUT" }};
static Setting::Handle<QString> desktopOutputDeviceSetting { QStringList { Audio::AUDIO, Audio::DESKTOP, "OUTPUT" }};
static Setting::Handle<QString> hmdInputDeviceSetting { QStringList { Audio::AUDIO, Audio::HMD, "INPUT" }};
static Setting::Handle<QString> hmdOutputDeviceSetting { QStringList { Audio::AUDIO, Audio::HMD, "OUTPUT" }};

Setting::Handle<QString>& getSetting(bool contextIsHMD, QAudio::Mode mode) {
    if (mode == QAudio::AudioInput) {
        return contextIsHMD ? hmdInputDeviceSetting : desktopInputDeviceSetting;
    } else { // if (mode == QAudio::AudioOutput)
        return contextIsHMD ? hmdOutputDeviceSetting : desktopOutputDeviceSetting;
    }
}

enum AudioDeviceRole {
    DisplayRole = Qt::DisplayRole,
    CheckStateRole = Qt::CheckStateRole,
    PeakRole = Qt::UserRole,
    InfoRole = Qt::UserRole + 1
};

QHash<int, QByteArray> AudioDeviceList::_roles {
    { DisplayRole, "display" },
    { CheckStateRole, "selected" },
    { PeakRole, "peak" },
    { InfoRole, "info" }
};

static QString getTargetDevice(bool hmd, QAudio::Mode mode) {
    QString deviceName;
    auto& setting = getSetting(hmd, mode);
    if (setting.isSet()) {
        deviceName = setting.get();
    } else if (hmd) {
        if (mode == QAudio::AudioInput) {
            deviceName = qApp->getActiveDisplayPlugin()->getPreferredAudioInDevice();
        } else { // if (_mode == QAudio::AudioOutput)
            deviceName = qApp->getActiveDisplayPlugin()->getPreferredAudioOutDevice();
        }
    }
    return deviceName;
}

Qt::ItemFlags AudioDeviceList::_flags { Qt::ItemIsSelectable | Qt::ItemIsEnabled };

QVariant AudioDeviceList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    if (role == DisplayRole) {
        return _devices.at(index.row())->display;
    } else if (role == CheckStateRole) {
        return _devices.at(index.row())->selected;
    } else if (role == InfoRole) {
        return QVariant::fromValue<QAudioDeviceInfo>(_devices.at(index.row())->info);
    } else {
        return QVariant();
    }
}

QVariant AudioInputDeviceList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    if (role == PeakRole) {
        return std::static_pointer_cast<AudioInputDevice>(_devices.at(index.row()))->peak;
    } else {
        return AudioDeviceList::data(index, role);
    }
}

void AudioDeviceList::resetDevice(bool contextIsHMD) {
    auto client = DependencyManager::get<AudioClient>().data();
    QString deviceName = getTargetDevice(contextIsHMD, _mode);
    // FIXME can't use blocking connections here, so we can't determine whether the switch succeeded or not
    // We need to have the AudioClient emit signals on switch success / failure
    QMetaObject::invokeMethod(client, "switchAudioDevice", 
        Q_ARG(QAudio::Mode, _mode), Q_ARG(QString, deviceName));

#if 0
    bool switchResult = false;
    QMetaObject::invokeMethod(client, "switchAudioDevice", Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, switchResult),
        Q_ARG(QAudio::Mode, _mode), Q_ARG(QString, deviceName));

    // try to set to the default device for this mode
    if (!switchResult) {
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
#endif
}

void AudioDeviceList::onDeviceChanged(const QAudioDeviceInfo& device) {
    auto oldDevice = _selectedDevice;
    _selectedDevice = device;

    for (auto i = 0; i < rowCount(); ++i) {
        AudioDevice& device = *_devices[i];

        if (device.selected && device.info != _selectedDevice) {
            device.selected = false;
        } else if (device.info == _selectedDevice) {
            device.selected = true;
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
        _devices.push_back(newDevice(device));
    }

    endResetModel();
}

bool AudioInputDeviceList::peakValuesAvailable() {
    std::call_once(_peakFlag, [&] {
        _peakValuesAvailable = DependencyManager::get<AudioClient>()->peakValuesAvailable();
    });
    return _peakValuesAvailable;
}

void AudioInputDeviceList::setPeakValuesEnabled(bool enable) {
    if (peakValuesAvailable() && (enable != _peakValuesEnabled)) {
        DependencyManager::get<AudioClient>()->enablePeakValues(enable);
        _peakValuesEnabled = enable;
        emit peakValuesEnabledChanged(_peakValuesEnabled);
    }
}

void AudioInputDeviceList::onPeakValueListChanged(const QList<float>& peakValueList) {
    assert(_mode == QAudio::AudioInput);

    if (peakValueList.length() != rowCount()) {
        qWarning() << "AudioDeviceList" << __FUNCTION__ << "length mismatch";
    }

    for (auto i = 0; i < rowCount(); ++i) {
        std::static_pointer_cast<AudioInputDevice>(_devices[i])->peak = peakValueList[i];
    }

    emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0), { PeakRole });
}

AudioDevices::AudioDevices(bool& contextIsHMD) : _contextIsHMD(contextIsHMD) {
    auto client = DependencyManager::get<AudioClient>();

    connect(client.data(), &AudioClient::deviceChanged, this, &AudioDevices::onDeviceChanged, Qt::QueuedConnection);
    connect(client.data(), &AudioClient::devicesChanged, this, &AudioDevices::onDevicesChanged, Qt::QueuedConnection);
    connect(client.data(), &AudioClient::peakValueListChanged, &_inputs, &AudioInputDeviceList::onPeakValueListChanged, Qt::QueuedConnection);

    // connections are made after client is initialized, so we must also fetch the devices
    _inputs.onDeviceChanged(client->getActiveAudioDevice(QAudio::AudioInput));
    _outputs.onDeviceChanged(client->getActiveAudioDevice(QAudio::AudioOutput));
    _inputs.onDevicesChanged(client->getAudioDevices(QAudio::AudioInput));
    _outputs.onDevicesChanged(client->getAudioDevices(QAudio::AudioOutput));
}

void AudioDevices::onContextChanged(const QString& context) {
    _inputs.resetDevice(_contextIsHMD);
    _outputs.resetDevice(_contextIsHMD);
}

void AudioDevices::onDeviceSelected(QAudio::Mode mode, const QAudioDeviceInfo& device, const QAudioDeviceInfo& previousDevice) {
    QString deviceName = device.isNull() ? QString() : device.deviceName();

    auto& setting = getSetting(_contextIsHMD, mode);

    // check for a previous device
    auto wasDefault = setting.get().isNull();

    // store the selected device
    setting.set(deviceName);

    // log the selected device
    if (!device.isNull()) {
        QJsonObject data;

        const QString MODE = "audio_mode";
        const QString INPUT = "INPUT";
        const QString OUTPUT = "OUTPUT"; data[MODE] = mode == QAudio::AudioInput ? INPUT : OUTPUT;

        const QString CONTEXT = "display_mode";
        data[CONTEXT] = _contextIsHMD ? Audio::HMD : Audio::DESKTOP;

        const QString DISPLAY = "display_device";
        data[DISPLAY] = qApp->getActiveDisplayPlugin()->getName();

        const QString DEVICE = "device";
        const QString PREVIOUS_DEVICE = "previous_device";
        const QString WAS_DEFAULT = "was_default";
        data[DEVICE] = deviceName;
        data[PREVIOUS_DEVICE] = previousDevice.deviceName();
        data[WAS_DEFAULT] = wasDefault;

        UserActivityLogger::getInstance().logAction("selected_audio_device", data);
    }
}

void AudioDevices::onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device) {
    if (mode == QAudio::AudioInput) {
        if (_requestedInputDevice == device) {
            onDeviceSelected(QAudio::AudioInput, device, _inputs._selectedDevice);
            _requestedInputDevice = QAudioDeviceInfo();
        }
        _inputs.onDeviceChanged(device);
    } else { // if (mode == QAudio::AudioOutput)
        if (_requestedOutputDevice == device) {
            onDeviceSelected(QAudio::AudioOutput, device, _outputs._selectedDevice);
            _requestedOutputDevice = QAudioDeviceInfo();
        }
        _outputs.onDeviceChanged(device);
    }
}

void AudioDevices::onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices) {
    static std::once_flag once;
    if (mode == QAudio::AudioInput) {
        _inputs.onDevicesChanged(devices);
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.onDevicesChanged(devices);
    }
    std::call_once(once, [&] { onContextChanged(QString()); });
}


void AudioDevices::chooseInputDevice(const QAudioDeviceInfo& device) {
    auto client = DependencyManager::get<AudioClient>();
    _requestedInputDevice = device;
    QMetaObject::invokeMethod(client.data(), "switchAudioDevice",
        Q_ARG(QAudio::Mode, QAudio::AudioInput),
        Q_ARG(const QAudioDeviceInfo&, device));
}

void AudioDevices::chooseOutputDevice(const QAudioDeviceInfo& device) {
    auto client = DependencyManager::get<AudioClient>();
    _requestedOutputDevice = device;
    QMetaObject::invokeMethod(client.data(), "switchAudioDevice",
        Q_ARG(QAudio::Mode, QAudio::AudioOutput),
        Q_ARG(const QAudioDeviceInfo&, device));
}
