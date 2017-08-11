//
//  AudioDevices.h
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 28/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_scripting_AudioDevices_h
#define hifi_scripting_AudioDevices_h

#include <QObject>
#include <QAbstractListModel>
#include <QAudioDeviceInfo>

namespace scripting {

class AudioDevice {
public:
    QAudioDeviceInfo info;
    QString display;
    bool selectedDesktop { false };
    bool selectedHMD { false };
};

class AudioDeviceList : public QAbstractListModel {
    Q_OBJECT

    enum AudioDeviceRoles {
        DeviceNameRole =  Qt::UserRole + 1,
        SelectedDesktopRole,
        SelectedHMDRole,
        DeviceInfoRole
    };

public:
    AudioDeviceList(QAudio::Mode mode);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override { Q_UNUSED(parent); return _devices.size(); }
    QHash<int, QByteArray> roleNames() const override { return _roles; }
    Qt::ItemFlags flags(const QModelIndex& index) const override { return _flags; }

    // get/set devices through a QML ListView
    QVariant data(const QModelIndex& index, int role) const override;

    // reset device to the last selected device in this context, or the default
    void resetDevice(bool contextIsHMD);

signals:
    void deviceChanged(const QAudioDeviceInfo& device);

private slots:
    void onDeviceChanged(const QAudioDeviceInfo& device, bool isHMD);
    void onDevicesChanged(const QList<QAudioDeviceInfo>& devices, bool isHMD);

private:
    friend class AudioDevices;

    static QHash<int, QByteArray> _roles;
    static Qt::ItemFlags _flags;
    const QAudio::Mode _mode;
    QAudioDeviceInfo _selectedDesktopDevice;
    QAudioDeviceInfo _selectedHMDDevice;
    QList<AudioDevice> _devices;
    QString _hmdSavedDeviceName;
    QString _desktopSavedDeviceName;
};

class Audio;

class AudioDevices : public QObject {
    Q_OBJECT
    Q_PROPERTY(AudioDeviceList* input READ getInputList NOTIFY nop)
    Q_PROPERTY(AudioDeviceList* output READ getOutputList NOTIFY nop)

public:
    AudioDevices(bool& contextIsHMD);
    void chooseInputDevice(const QAudioDeviceInfo& device, bool isHMD);
    void chooseOutputDevice(const QAudioDeviceInfo& device, bool isHMD);

signals:
    void nop();

private slots:
    void onContextChanged(const QString& context);
    void onDeviceSelected(QAudio::Mode mode, const QAudioDeviceInfo& device,
                          const QAudioDeviceInfo& previousDevice, bool isHMD);
    void onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device);
    void onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices);

private:
    friend class Audio;

    AudioDeviceList* getInputList() { return &_inputs; }
    AudioDeviceList* getOutputList() { return &_outputs; }

    AudioDeviceList _inputs { QAudio::AudioInput };
    AudioDeviceList _outputs { QAudio::AudioOutput };
    QAudioDeviceInfo _requestedOutputDevice;
    QAudioDeviceInfo _requestedInputDevice;

    const bool& _contextIsHMD;
};

};

#endif // hifi_scripting_AudioDevices_h
