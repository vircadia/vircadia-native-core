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

#include <memory>
#include <mutex>

#include <QObject>
#include <QAbstractListModel>
#include <QAudioDeviceInfo>

namespace scripting {

class AudioDevice {
public:
    QAudioDeviceInfo info;
    QString display;
    bool selected { false };
};

class AudioDeviceList : public QAbstractListModel {
    Q_OBJECT

public:
    AudioDeviceList(QAudio::Mode mode = QAudio::AudioOutput) : _mode(mode) {}
    ~AudioDeviceList() = default;

    virtual std::shared_ptr<AudioDevice> newDevice(const AudioDevice& device)
        { return std::make_shared<AudioDevice>(device); }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override { Q_UNUSED(parent); return _devices.size(); }
    QHash<int, QByteArray> roleNames() const override { return _roles; }
    Qt::ItemFlags flags(const QModelIndex& index) const override { return _flags; }

    // get/set devices through a QML ListView
    QVariant data(const QModelIndex& index, int role) const override;

    // reset device to the last selected device in this context, or the default
    void resetDevice(bool contextIsHMD);

signals:
    void deviceChanged(const QAudioDeviceInfo& device);

protected slots:
    void onDeviceChanged(const QAudioDeviceInfo& device);
    void onDevicesChanged(const QList<QAudioDeviceInfo>& devices);

protected:
    friend class AudioDevices;

    static QHash<int, QByteArray> _roles;
    static Qt::ItemFlags _flags;
    const QAudio::Mode _mode;
    QAudioDeviceInfo _selectedDevice;
    QList<std::shared_ptr<AudioDevice>> _devices;
};

class AudioInputDevice : public AudioDevice {
public:
    AudioInputDevice(const AudioDevice& device) : AudioDevice(device) {}
    float peak { 0.0f };
};

class AudioInputDeviceList : public AudioDeviceList {
    Q_OBJECT
    Q_PROPERTY(bool peakValuesAvailable READ peakValuesAvailable)
    Q_PROPERTY(bool peakValuesEnabled READ peakValuesEnabled WRITE setPeakValuesEnabled NOTIFY peakValuesEnabledChanged)

public:
    AudioInputDeviceList() : AudioDeviceList(QAudio::AudioInput) {}
    virtual ~AudioInputDeviceList() = default;

    virtual std::shared_ptr<AudioDevice> newDevice(const AudioDevice& device) override
        { return std::make_shared<AudioInputDevice>(device); }

    QVariant data(const QModelIndex& index, int role) const override;

signals:
    void peakValuesEnabledChanged(bool enabled);

protected slots:
    void onPeakValueListChanged(const QList<float>& peakValueList);

protected:
    friend class AudioDevices;

    bool peakValuesAvailable();
    std::once_flag _peakFlag;
    bool _peakValuesAvailable;

    bool peakValuesEnabled() const { return _peakValuesEnabled; }
    void setPeakValuesEnabled(bool enable);
    bool _peakValuesEnabled { false };
};

class Audio;

class AudioDevices : public QObject {
    Q_OBJECT
    Q_PROPERTY(AudioInputDeviceList* input READ getInputList NOTIFY nop)
    Q_PROPERTY(AudioDeviceList* output READ getOutputList NOTIFY nop)

public:
    AudioDevices(bool& contextIsHMD);
    void chooseInputDevice(const QAudioDeviceInfo& device);
    void chooseOutputDevice(const QAudioDeviceInfo& device);

signals:
    void nop();

private slots:
    void onContextChanged(const QString& context);
    void onDeviceSelected(QAudio::Mode mode, const QAudioDeviceInfo& device, const QAudioDeviceInfo& previousDevice);
    void onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device);
    void onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices);

private:
    friend class Audio;

    AudioInputDeviceList* getInputList() { return &_inputs; }
    AudioDeviceList* getOutputList() { return &_outputs; }

    AudioInputDeviceList _inputs;
    AudioDeviceList _outputs { QAudio::AudioOutput };
    QAudioDeviceInfo _requestedOutputDevice;
    QAudioDeviceInfo _requestedInputDevice;

    const bool& _contextIsHMD;
};

};

#endif // hifi_scripting_AudioDevices_h
