//
//  AudioDeviceScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 3/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioDeviceScriptingInterface_h
#define hifi_AudioDeviceScriptingInterface_h

#include <QObject>
#include <QString>
#include <QVector>
#include <QAbstractListModel>
#include <QAudio>

class AudioEffectOptions;

struct ScriptingAudioDeviceInfo {
    QString name;
    bool selected;
    QAudio::Mode mode;
};

class AudioDeviceScriptingInterface : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QStringList inputAudioDevices READ inputAudioDevices NOTIFY inputAudioDevicesChanged)
    Q_PROPERTY(QStringList outputAudioDevices READ outputAudioDevices NOTIFY outputAudioDevicesChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)

public:
    static AudioDeviceScriptingInterface* getInstance();

    QStringList inputAudioDevices() const;
    QStringList outputAudioDevices() const;
    bool muted();

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    enum Roles {
        DisplayNameRole = Qt::UserRole,
        SelectedRole,
        AudioModeRole
    };

private slots:
    void onDeviceChanged();
    void onCurrentInputDeviceChanged(const QString& name);
    void onCurrentOutputDeviceChanged(const QString& name);
    void currentDeviceUpdate(const QString& name, QAudio::Mode mode);

public slots:
    bool setInputDevice(const QString& deviceName);
    bool setOutputDevice(const QString& deviceName);
    bool setDeviceFromMenu(const QString& deviceMenuName);

    QString getInputDevice();
    QString getOutputDevice();

    QString getDefaultInputDevice();
    QString getDefaultOutputDevice();

    QVector<QString> getInputDevices();
    QVector<QString> getOutputDevices();

    float getInputVolume();
    void setInputVolume(float volume);
    void setReverb(bool reverb);
    void setReverbOptions(const AudioEffectOptions* options);

    bool getMuted();
    void toggleMute();
    
    void setMuted(bool muted);

    void setInputDeviceAsync(const QString& deviceName);
    void setOutputDeviceAsync(const QString& deviceName);
private:
    AudioDeviceScriptingInterface();

signals:
    void muteToggled();
    void deviceChanged();
    void currentInputDeviceChanged(const QString& name);
    void currentOutputDeviceChanged(const QString& name);
    void mutedChanged(bool muted);
    void inputAudioDevicesChanged(QStringList inputAudioDevices);
    void outputAudioDevicesChanged(QStringList outputAudioDevices);

private:
    QVector<ScriptingAudioDeviceInfo> _devices;

    QStringList _inputAudioDevices;
    QStringList _outputAudioDevices;

    QString _currentInputDevice;
    QString _currentOutputDevice;
};

#endif // hifi_AudioDeviceScriptingInterface_h
