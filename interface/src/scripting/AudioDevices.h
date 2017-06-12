//
//  AudioDeviceScriptingInterface.h
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
    QString name;
    bool selected { false };
};

class AudioDeviceList : public QAbstractListModel {
    Q_OBJECT

public:
    AudioDeviceList(QAudio::Mode mode);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant &value, int role) override;

    void setDevice(const QAudioDeviceInfo& device);
    void populate(const QList<QAudioDeviceInfo>& devices);

signals:
    void deviceChanged();

private:
    static QHash<int, QByteArray> _roles;
    static Qt::ItemFlags _flags;

    QAudio::Mode _mode;
    QAudioDeviceInfo _selectedDevice;
    QModelIndex _selectedDeviceIndex;
    QList<AudioDevice> _devices;
};

class AudioDevices : public QObject {
    Q_OBJECT
    Q_PROPERTY(AudioDeviceList* input READ getInputList NOTIFY nop)
    Q_PROPERTY(AudioDeviceList* output READ getOutputList NOTIFY nop)

public:
    AudioDevices();
    void restoreDevices(bool isHMD);

signals:
    void nop();

private slots:
    void onDeviceChanged(QAudio::Mode mode, const QAudioDeviceInfo& device);
    void onDevicesChanged(QAudio::Mode mode, const QList<QAudioDeviceInfo>& devices);

private:
    friend class Audio;

    AudioDeviceList* getInputList() { return &_inputs; }
    AudioDeviceList* getOutputList() { return &_outputs; }

    AudioDeviceList _inputs { QAudio::AudioInput };
    AudioDeviceList _outputs { QAudio::AudioOutput };

    bool tester;
};

};

#endif // hifi_scripting_AudioDevices_h
