//
//  Audio.h
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 28/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_scripting_Audio_h
#define hifi_scripting_Audio_h

#include "AudioScriptingInterface.h"
#include "AudioDevices.h"
#include "AudioEffectOptions.h"
#include "SettingHandle.h"
#include "AudioFileWav.h"
#include <shared/ReadWriteLockable.h>

namespace scripting {

class Audio : public AudioScriptingInterface, protected ReadWriteLockable {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    /**jsdoc
     * The Audio API features tools to help control audio contexts and settings.
     *
     * @namespace Audio
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-server-entity
     * @hifi-assignment-client
     *
     * @property {boolean} muted
     * @property {boolean} noiseReduction
     * @property {number} inputVolume
     * @property {number} inputLevel <em>Read-only.</em>
     * @property {string} context <em>Read-only.</em>
     * @property {} devices <em>Read-only.</em>
     */
 
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool noiseReduction READ noiseReductionEnabled WRITE enableNoiseReduction NOTIFY noiseReductionChanged)
    Q_PROPERTY(float inputVolume READ getInputVolume WRITE setInputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(float inputLevel READ getInputLevel NOTIFY inputLevelChanged)
    Q_PROPERTY(QString context READ getContext NOTIFY contextChanged)
    Q_PROPERTY(AudioDevices* devices READ getDevices NOTIFY nop)

public:
    static QString AUDIO;
    static QString HMD;
    static QString DESKTOP;

    static float loudnessToLevel(float loudness);

    virtual ~Audio() {}

    bool isMuted() const;
    bool noiseReductionEnabled() const;
    float getInputVolume() const;
    float getInputLevel() const;
    QString getContext() const;

    void showMicMeter(bool show);

    /**jsdoc
     * @function Audio.setInputDevice
     * @param {} device
     * @param {boolean} isHMD 
     */
    Q_INVOKABLE void setInputDevice(const QAudioDeviceInfo& device, bool isHMD);

    /**jsdoc
     * @function Audio.setOutputDevice
     * @param {} device
     * @param {boolean} isHMD
     */
    Q_INVOKABLE void setOutputDevice(const QAudioDeviceInfo& device, bool isHMD);

    /**jsdoc
     * @function Audio.setReverb
     * @param {boolean} enable
     */
    Q_INVOKABLE void setReverb(bool enable);
    
    /**jsdoc
     * @function Audio.setReverbOptions
     * @param {} options
     */
    Q_INVOKABLE void setReverbOptions(const AudioEffectOptions* options);
   
    /**jsdoc
     * @function Audio.startRecording
     * @param {string} filename
     * @returns {boolean}
     */
    Q_INVOKABLE bool startRecording(const QString& filename);
    
    /**jsdoc
     * @function Audio.stopRecording
     */
    Q_INVOKABLE void stopRecording();

    /**jsdoc
     * @function Audio.getRecording
     * @returns {boolean}
     */
    Q_INVOKABLE bool getRecording();

signals:

    /**jsdoc
     * @function Audio.nop
     * @returns {Signal}
     */
    void nop();

    /**jsdoc
     * @function Audio.mutedChanged
     * @param {boolean} isMuted
     * @returns {Signal}
     */
    void mutedChanged(bool isMuted);
    
    /**jsdoc
     * @function Audio.noiseReductionChanged
     * @param {boolean} isEnabled
     * @returns {Signal}
     */
    void noiseReductionChanged(bool isEnabled);

    /**jsdoc
     * @function Audio.inputVolumeChanged
     * @param {number} volume
     * @returns {Signal}
     */
    void inputVolumeChanged(float volume);

    /**jsdoc
     * @function Audio.inputLevelChanged
     * @param {number} level
     * @returns {Signal}
     */
    void inputLevelChanged(float level);

    /**jsdoc
     * @function Audio.contextChanged
     * @param {string} context
     * @returns {Signal}
     */
    void contextChanged(const QString& context);

public slots:

    /**jsdoc
     * @function Audio.onContextChanged
     * @returns {Signal}
     */
    void onContextChanged();

private slots:
    void setMuted(bool muted);
    void enableNoiseReduction(bool enable);
    void setInputVolume(float volume);
    void onInputLoudnessChanged(float loudness);

protected:
    // Audio must live on a separate thread from AudioClient to avoid deadlocks
    Audio();

private:

    float _inputVolume { 1.0f };
    float _inputLevel { 0.0f };
    bool _isMuted { false };
    bool _enableNoiseReduction { true };  // Match default value of AudioClient::_isNoiseGateEnabled.
    bool _contextIsHMD { false };
    AudioDevices* getDevices() { return &_devices; }
    AudioDevices _devices;
};

};

#endif // hifi_scripting_Audio_h
