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

#include <map>
#include <algorithm>

#include <shared/QtHelpers.h>
#include <plugins/DisplayPlugin.h>

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
    DeviceNameRole = Qt::UserRole,
    SelectedDesktopRole,
    SelectedHMDRole,
    PeakRole,
    InfoRole
};

QHash<int, QByteArray> AudioDeviceList::_roles {
    { DeviceNameRole, "devicename" },
    { SelectedDesktopRole, "selectedDesktop" },
    { SelectedHMDRole, "selectedHMD" },
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

AudioDeviceList::AudioDeviceList(QAudio::Mode mode) : _mode(mode) {
    auto& setting1 = getSetting(true, QAudio::AudioInput);
    if (setting1.isSet()) {
        qDebug() << "Device name in settings for HMD, Input" << setting1.get();
    } else {
        qDebug() << "Device name in settings for HMD, Input not set";
    }

    auto& setting2 = getSetting(true, QAudio::AudioOutput);
    if (setting2.isSet()) {
        qDebug() << "Device name in settings for HMD, Output" << setting2.get();
    } else {
        qDebug() << "Device name in settings for HMD, Output not set";
    }

    auto& setting3 = getSetting(false, QAudio::AudioInput);
    if (setting3.isSet()) {
        qDebug() << "Device name in settings for Desktop, Input" << setting3.get();
    } else {
        qDebug() << "Device name in settings for Desktop, Input not set";
    }

    auto& setting4 = getSetting(false, QAudio::AudioOutput);
    if (setting4.isSet()) {
        qDebug() << "Device name in settings for Desktop, Output" << setting4.get();
    } else {
        qDebug() << "Device name in settings for Desktop, Output not set";
    }
}

AudioDeviceList::~AudioDeviceList() {
    //save all selected devices
    auto& settingHMD = getSetting(true, _mode);
    auto& settingDesktop = getSetting(false, _mode);
    // store the selected device
    foreach(std::shared_ptr<AudioDevice> adevice, _devices) {
        if (adevice->selectedDesktop) {
            settingDesktop.set(adevice->info.deviceName());
        }
        if (adevice->selectedHMD) {
            settingHMD.set(adevice->info.deviceName());
        }
    }
}

QVariant AudioDeviceList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    if (role == DeviceNameRole) {
        return _devices.at(index.row())->display;
    } else if (role == SelectedDesktopRole) {
        return _devices.at(index.row())->selectedDesktop;
    } else if (role == SelectedHMDRole) {
        return _devices.at(index.row())->selectedHMD;
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

void AudioDeviceList::onDeviceChanged(const QAudioDeviceInfo& device, bool isHMD) {
    QAudioDeviceInfo& selectedDevice = isHMD ? _selectedHMDDevice : _selectedDesktopDevice;
    selectedDevice = device;

    for (auto i = 0; i < _devices.size(); ++i) {
        std::shared_ptr<AudioDevice> device = _devices[i];
        bool& isSelected = isHMD ? device->selectedHMD : device->selectedDesktop;
        if (isSelected && device->info != selectedDevice) {
            isSelected = false;
        } else if (device->info == selectedDevice) {
            isSelected = true;
        }
    }

    emit deviceChanged(selectedDevice);
    emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

// Function returns 'strings similarity' as a number. The lesser number - the more similar strings are. Absolutely equal strings should return 0.
// Optimized version kindly provided by Ken
int levenshteinDistance(const QString& s1, const QString& s2) {
    const int m = s1.size();
    const int n = s2.size();

    if (m == 0) {
        return n;
    }
    if (n == 0) {
        return m;
    }

    auto cost = (int*)alloca((n + 1) * sizeof(int));

    for (int j = 0; j <= n; j++) {
        cost[j] = j;
    }

    for (int i = 0; i < m; i++) {

        int prev = i;
        cost[0] = i + 1;

        for (int j = 0; j < n; j++) {

            int temp = cost[j + 1];
            cost[j + 1] = (s1[i] == s2[j]) ? prev : std::min(cost[j], std::min(temp, prev)) + 1;
            prev = temp;
        }
    }
    return cost[n];
}

std::shared_ptr<scripting::AudioDevice> getSimilarDevice(const QString& deviceName, const QList<std::shared_ptr<scripting::AudioDevice>>& devices) {

    int minDistance = INT_MAX;
    int minDistanceIndex = 0;

    for (auto i = 0; i < devices.length(); ++i) {
        auto distance = levenshteinDistance(deviceName, devices[i]->info.deviceName());
        if (distance < minDistance) {
            minDistance = distance;
            minDistanceIndex = i;
        }
    }

    return devices[minDistanceIndex];
}

void AudioDeviceList::onDevicesChanged(const QList<QAudioDeviceInfo>& devices) {
    beginResetModel();

    QList<std::shared_ptr<AudioDevice>> newDevices;
    bool hmdIsSelected = false;
    bool desktopIsSelected = false;

    foreach(const QAudioDeviceInfo& deviceInfo, devices) {
        for (bool isHMD : {false, true}) {
            auto& backupSelectedDeviceName = isHMD ? _backupSelectedHMDDeviceName : _backupSelectedDesktopDeviceName;
            if (deviceInfo.deviceName() == backupSelectedDeviceName) {
                QAudioDeviceInfo& selectedDevice = isHMD ? _selectedHMDDevice : _selectedDesktopDevice;
                selectedDevice = deviceInfo;
                backupSelectedDeviceName.clear();
            }
        }
    }

    foreach(const QAudioDeviceInfo& deviceInfo, devices) {
        AudioDevice device;
        device.info = deviceInfo;
        device.display = device.info.deviceName()
            .replace("High Definition", "HD")
            .remove("Device")
            .replace(" )", ")");

        for (bool isHMD : {false, true}) {
            QAudioDeviceInfo& selectedDevice = isHMD ? _selectedHMDDevice : _selectedDesktopDevice;
            bool& isSelected = isHMD ? device.selectedHMD : device.selectedDesktop;

            if (!selectedDevice.isNull()) {
                isSelected = (device.info == selectedDevice);
            }
            else {
                //no selected device for context. fallback to saved
                const QString& savedDeviceName = isHMD ? _hmdSavedDeviceName : _desktopSavedDeviceName;
                isSelected = (device.info.deviceName() == savedDeviceName);
            }

            if (isSelected) {
                if (isHMD) {
                    hmdIsSelected = isSelected;
                } else {
                    desktopIsSelected = isSelected;
                }

                // check if this device *is not* in old devices list - it means it was just re-plugged so needs to be selected explicitly
                bool isNewDevice = true;
                for (auto& oldDevice : _devices) {
                    if (oldDevice->info.deviceName() == device.info.deviceName()) {
                        isNewDevice = false;
                        break;
                    }
                }

                if (isNewDevice) {
                    emit selectedDevicePlugged(device.info, isHMD);
                }
            }
        }

        newDevices.push_back(newDevice(device));
    }

    if (!newDevices.isEmpty()) {
        if (!hmdIsSelected) {
            _backupSelectedHMDDeviceName = !_selectedHMDDevice.isNull() ? _selectedHMDDevice.deviceName() : _hmdSavedDeviceName;
            auto device = getSimilarDevice(_backupSelectedHMDDeviceName, newDevices);
            device->selectedHMD = true;
            emit selectedDevicePlugged(device->info, true);
        }
        if (!desktopIsSelected) {
            _backupSelectedDesktopDeviceName = !_selectedDesktopDevice.isNull() ? _selectedDesktopDevice.deviceName() : _desktopSavedDeviceName;
            auto device = getSimilarDevice(_backupSelectedDesktopDeviceName, newDevices);
            device->selectedDesktop = true;
            emit selectedDevicePlugged(device->info, false);
        }
    }

    _devices.swap(newDevices);
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
    auto client = DependencyManager::get<AudioClient>().data();

    connect(client, &AudioClient::deviceChanged, this, &AudioDevices::onDeviceChanged, Qt::QueuedConnection);
    connect(client, &AudioClient::devicesChanged, this, &AudioDevices::onDevicesChanged, Qt::QueuedConnection);
    connect(client, &AudioClient::peakValueListChanged, &_inputs, &AudioInputDeviceList::onPeakValueListChanged, Qt::QueuedConnection);

    _inputs.onDeviceChanged(client->getActiveAudioDevice(QAudio::AudioInput), contextIsHMD);
    _outputs.onDeviceChanged(client->getActiveAudioDevice(QAudio::AudioOutput), contextIsHMD);

    // connections are made after client is initialized, so we must also fetch the devices
    const QList<QAudioDeviceInfo>& devicesInput = client->getAudioDevices(QAudio::AudioInput);
    const QList<QAudioDeviceInfo>& devicesOutput = client->getAudioDevices(QAudio::AudioOutput);

    //setup devices
    _inputs.onDevicesChanged(devicesInput);
    _outputs.onDevicesChanged(devicesOutput);
}

AudioDevices::~AudioDevices() {}

void AudioDevices::onContextChanged(const QString& context) {
    _inputs.resetDevice(_contextIsHMD);
    _outputs.resetDevice(_contextIsHMD);
}

void AudioDevices::onDeviceSelected(QAudio::Mode mode, const QAudioDeviceInfo& device,
                                    const QAudioDeviceInfo& previousDevice, bool isHMD) {
    QString deviceName = device.isNull() ? QString() : device.deviceName();

    auto& setting = getSetting(isHMD, mode);

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
            onDeviceSelected(QAudio::AudioInput, device,
                             _contextIsHMD ? _inputs._selectedHMDDevice : _inputs._selectedDesktopDevice,
                             _contextIsHMD);
            _requestedInputDevice = QAudioDeviceInfo();
        }
        _inputs.onDeviceChanged(device, _contextIsHMD);
    } else { // if (mode == QAudio::AudioOutput)
        if (_requestedOutputDevice == device) {
            onDeviceSelected(QAudio::AudioOutput, device,
                             _contextIsHMD ? _outputs._selectedHMDDevice : _outputs._selectedDesktopDevice,
                             _contextIsHMD);
            _requestedOutputDevice = QAudioDeviceInfo();
        }
        _outputs.onDeviceChanged(device, _contextIsHMD);
    }
}

void AudioDevices::onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices) {
    static std::once_flag once;
    std::call_once(once, [&] {
        //readout settings
        auto client = DependencyManager::get<AudioClient>().data();

        _inputs._hmdSavedDeviceName = getTargetDevice(true, QAudio::AudioInput);
        _inputs._desktopSavedDeviceName = getTargetDevice(false, QAudio::AudioInput);

        //fallback to default device
        if (_inputs._desktopSavedDeviceName.isEmpty()) {
            _inputs._desktopSavedDeviceName = client->getActiveAudioDevice(QAudio::AudioInput).deviceName();
        }
        //fallback to desktop device
        if (_inputs._hmdSavedDeviceName.isEmpty()) {
            _inputs._hmdSavedDeviceName = _inputs._desktopSavedDeviceName;
        }

        _outputs._hmdSavedDeviceName = getTargetDevice(true, QAudio::AudioOutput);
        _outputs._desktopSavedDeviceName = getTargetDevice(false, QAudio::AudioOutput);

        if (_outputs._desktopSavedDeviceName.isEmpty()) {
            _outputs._desktopSavedDeviceName = client->getActiveAudioDevice(QAudio::AudioOutput).deviceName();
        }
        if (_outputs._hmdSavedDeviceName.isEmpty()) {
            _outputs._hmdSavedDeviceName = _outputs._desktopSavedDeviceName;
        }
        onContextChanged(QString());
    });

    //set devices for both contexts
    if (mode == QAudio::AudioInput) {
        _inputs.onDevicesChanged(devices);

        static std::once_flag onceAfterInputDevicesChanged;
        std::call_once(onceAfterInputDevicesChanged, [&] { // we only want 'selectedDevicePlugged' signal to be handled after initial list of input devices was populated
            connect(&_inputs, &AudioDeviceList::selectedDevicePlugged, this, &AudioDevices::chooseInputDevice);
        });
    } else { // if (mode == QAudio::AudioOutput)
        _outputs.onDevicesChanged(devices);

        static std::once_flag onceAfterOutputDevicesChanged;
        std::call_once(onceAfterOutputDevicesChanged, [&] { // we only want 'selectedDevicePlugged' signal to be handled after initial list of output devices was populated
            connect(&_outputs, &AudioDeviceList::selectedDevicePlugged, this, &AudioDevices::chooseOutputDevice);
        });
    }
}


void AudioDevices::chooseInputDevice(const QAudioDeviceInfo& device, bool isHMD) {
    //check if current context equals device to change
    if (_contextIsHMD == isHMD) {
        auto client = DependencyManager::get<AudioClient>().data();
        _requestedInputDevice = device;
        QMetaObject::invokeMethod(client, "switchAudioDevice",
                                  Q_ARG(QAudio::Mode, QAudio::AudioInput),
                                  Q_ARG(const QAudioDeviceInfo&, device));
    } else {
        //context is different. just save device in settings
        onDeviceSelected(QAudio::AudioInput, device,
                         isHMD ? _inputs._selectedHMDDevice : _inputs._selectedDesktopDevice,
                         isHMD);
        _inputs.onDeviceChanged(device, isHMD);
    }
}

void AudioDevices::chooseOutputDevice(const QAudioDeviceInfo& device, bool isHMD) {
    //check if current context equals device to change
    if (_contextIsHMD == isHMD) {
        auto client = DependencyManager::get<AudioClient>().data();
        _requestedOutputDevice = device;
        QMetaObject::invokeMethod(client, "switchAudioDevice",
                                  Q_ARG(QAudio::Mode, QAudio::AudioOutput),
                                  Q_ARG(const QAudioDeviceInfo&, device));
    } else {
        //context is different. just save device in settings
        onDeviceSelected(QAudio::AudioOutput, device,
                         isHMD ? _outputs._selectedHMDDevice : _outputs._selectedDesktopDevice,
                         isHMD);
        _outputs.onDeviceChanged(device, isHMD);
    }
}
