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
     * The <code>Audio</code> API ... TODO
     *
     * @namespace Audio
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-server-entity
     * @hifi-assignment-client
     *
     * @property {boolean} muted - <code>true</code> if TODO, otherwise <code>false</code>.
     * @property {boolean} noiseReduction - <code>true</code> if TODO, otherwise <code>false</code>.
     * @property {number} inputVolume - TODO
     * @property {number} inputLevel - TODO <em>Read-only.</em>
     * @property {string} context - TODO <em>Read-only.</em>
     * @property {object} devices <em>Read-only.</em> <strong>Deprecated:</strong> This property is deprecated and will be 
     *     removed.
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
     * @param {object} device
     * @param {boolean} isHMD 
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE void setInputDevice(const QAudioDeviceInfo& device, bool isHMD);

    /**jsdoc
     * @function Audio.setOutputDevice
     * @param {object} device
     * @param {boolean} isHMD
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE void setOutputDevice(const QAudioDeviceInfo& device, bool isHMD);

    /**jsdoc
     * TODO
     * @function Audio.setReverb
     * @param {boolean} enable - TODO
     */
    Q_INVOKABLE void setReverb(bool enable);
    
    /**jsdoc
     * TODO
     * @function Audio.setReverbOptions
     * @param {AudioEffectOptions} options - TODO
     */
    Q_INVOKABLE void setReverbOptions(const AudioEffectOptions* options);
   
    /**jsdoc
     * TODO
     * @function Audio.startRecording
     * @param {string} filename - TODO
     * @returns {boolean} <code>true</code> if TODO, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool startRecording(const QString& filename);
    
    /**jsdoc
     * TODO
     * @function Audio.stopRecording
     */
    Q_INVOKABLE void stopRecording();

    /**jsdoc
     * TODO
     * @function Audio.getRecording
     * @returns {boolean} <code>true</code> if TODO, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool getRecording();

signals:

    /**jsdoc
     * Triggered when ... TODO
     * @function Audio.nop
     * @returns {Signal}
     */
    void nop();

    /**jsdoc
     * Triggered when ... TODO
     * @function Audio.mutedChanged
     * @param {boolean} isMuted - TODO
     * @returns {Signal}
     */
    void mutedChanged(bool isMuted);
    
    /**jsdoc
     * Triggered when ... TODO
     * @function Audio.noiseReductionChanged
     * @param {boolean} isEnabled - TODO
     * @returns {Signal}
     */
    void noiseReductionChanged(bool isEnabled);

    /**jsdoc
     * Triggered when ... TODO
     * @function Audio.inputVolumeChanged
     * @param {number} volume - TODO
     * @returns {Signal}
     */
    void inputVolumeChanged(float volume);

    /**jsdoc
     * Triggered when ... TODO
     * @function Audio.inputLevelChanged
     * @param {number} level - TODO
     * @returns {Signal}
     */
    void inputLevelChanged(float level);

    /**jsdoc
     * Triggered when ... TODO
     * @function Audio.contextChanged
     * @param {string} context - TODO
     * @returns {Signal}
     */
    void contextChanged(const QString& context);

public slots:

    /**jsdoc
     * @function Audio.onContextChanged
     * @deprecated This function is deprecated and will be removed.
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
