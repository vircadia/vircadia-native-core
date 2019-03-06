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

#include <functional>
#include "AudioScriptingInterface.h"
#include "AudioDevices.h"
#include "AudioEffectOptions.h"
#include "SettingHandle.h"
#include "AudioFileWav.h"
#include <shared/ReadWriteLockable.h>

using MutedGetter = std::function<bool()>;
using MutedSetter = std::function<void(bool)>;

namespace scripting {

class Audio : public AudioScriptingInterface, protected ReadWriteLockable {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    /**jsdoc
     * The <code>Audio</code> API provides facilities to interact with audio inputs and outputs and to play sounds.
     *
     * @namespace Audio
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     * @hifi-server-entity
     * @hifi-assignment-client
     *
     * @property {boolean} muted - <code>true</code> if the audio input is muted, otherwise <code>false</code>.
     * @property {boolean} noiseReduction - <code>true</code> if noise reduction is enabled, otherwise <code>false</code>. When 
     *     enabled, the input audio signal is blocked (fully attenuated) when it falls below an adaptive threshold set just 
     *     above the noise floor.
     * @property {number} inputLevel - The loudness of the audio input, range <code>0.0</code> (no sound) &ndash; 
     *     <code>1.0</code> (the onset of clipping). <em>Read-only.</em>
     * @property {boolean} clipping - <code>true</code> if the audio input is clipping, otherwise <code>false</code>.
     * @property {number} inputVolume - Adjusts the volume of the input audio; range <code>0.0</code> &ndash; <code>1.0</code>. 
     *     If set to a value, the resulting value depends on the input device: for example, the volume can't be changed on some 
     *     devices, and others might only support values of <code>0.0</code> and <code>1.0</code>.
     * @property {boolean} isStereoInput - <code>true</code> if the input audio is being used in stereo, otherwise 
     *     <code>false</code>. Some devices do not support stereo, in which case the value is always <code>false</code>.
     * @property {string} context - The current context of the audio: either <code>"Desktop"</code> or <code>"HMD"</code>.
     *     <em>Read-only.</em>
     * @property {object} devices <em>Read-only.</em> <strong>Deprecated:</strong> This property is deprecated and will be
     *     removed.
     * @property {boolean} isSoloing <em>Read-only.</em> <code>true</code> if any nodes are soloed.
     * @property {Uuid[]} soloList <em>Read-only.</em> Get the list of currently soloed node UUIDs.
     */

    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool noiseReduction READ noiseReductionEnabled WRITE enableNoiseReduction NOTIFY noiseReductionChanged)
    Q_PROPERTY(float inputVolume READ getInputVolume WRITE setInputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(float inputLevel READ getInputLevel NOTIFY inputLevelChanged)
    Q_PROPERTY(bool clipping READ isClipping NOTIFY clippingChanged)
    Q_PROPERTY(QString context READ getContext NOTIFY contextChanged)
    Q_PROPERTY(AudioDevices* devices READ getDevices NOTIFY nop)
    Q_PROPERTY(bool desktopMuted READ getMutedDesktop WRITE setMutedDesktop NOTIFY desktopMutedChanged)
    Q_PROPERTY(bool hmdMuted READ getMutedHMD WRITE setMutedHMD NOTIFY hmdMutedChanged)
    Q_PROPERTY(bool pushToTalk READ getPTT WRITE setPTT NOTIFY pushToTalkChanged);
    Q_PROPERTY(bool pushToTalkDesktop READ getPTTDesktop WRITE setPTTDesktop NOTIFY pushToTalkDesktopChanged)
    Q_PROPERTY(bool pushToTalkHMD READ getPTTHMD WRITE setPTTHMD NOTIFY pushToTalkHMDChanged)
    Q_PROPERTY(bool pushingToTalk READ getPushingToTalk NOTIFY pushingToTalkChanged)

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
    bool isClipping() const;
    QString getContext() const;

    void showMicMeter(bool show);

    // Mute setting setters and getters
    void setMutedDesktop(bool isMuted);
    bool getMutedDesktop() const;
    void setMutedHMD(bool isMuted);
    bool getMutedHMD() const;
    void setPTT(bool enabled);
    bool getPTT();
    void setPushingToTalk(bool pushingToTalk);
    bool getPushingToTalk() const;

    // Push-To-Talk setters and getters
    void setPTTDesktop(bool enabled);
    bool getPTTDesktop() const;
    void setPTTHMD(bool enabled);
    bool getPTTHMD() const;

    // Settings handlers
    void saveData();
    void loadData();

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
     * Enable or disable reverberation. Reverberation is done by the client, on the post-mix audio. The reverberation options 
     * come from either the domain's audio zone if used &mdash; configured on the server &mdash; or as scripted by 
     * {@link Audio.setReverbOptions|setReverbOptions}.
     * @function Audio.setReverb
     * @param {boolean} enable - <code>true</code> to enable reverberation, <code>false</code> to disable.
     * @example <caption>Enable reverberation for a short while.</caption>
     * var sound = SoundCache.getSound(Script.resourcesPath() + "sounds/sample.wav");
     * var injector;
     * var injectorOptions = {
     *     position: MyAvatar.position
     * };
     * 
     * Script.setTimeout(function () {
     *     print("Reverb OFF");
     *     Audio.setReverb(false);
     *     injector = Audio.playSound(sound, injectorOptions);
     * }, 1000);
     * 
     * Script.setTimeout(function () {
     *     var reverbOptions = new AudioEffectOptions();
     *     reverbOptions.roomSize = 100;
     *     Audio.setReverbOptions(reverbOptions);
     *     print("Reverb ON");
     *     Audio.setReverb(true);
     * }, 4000);
     * 
     * Script.setTimeout(function () {
     *     print("Reverb OFF");
     *     Audio.setReverb(false);
     * }, 8000);     */
    Q_INVOKABLE void setReverb(bool enable);
    
    /**jsdoc
     * Configure reverberation options. Use {@link Audio.setReverb|setReverb} to enable or disable reverberation.
     * @function Audio.setReverbOptions
     * @param {AudioEffectOptions} options - The reverberation options.
     */
    Q_INVOKABLE void setReverbOptions(const AudioEffectOptions* options);
   
    /**jsdoc
     * Starts making an audio recording of the audio being played in-world (i.e., not local-only audio) to a file in WAV format.
     * @function Audio.startRecording
     * @param {string} filename - The path and name of the file to make the recording in. Should have a <code>.wav</code> 
     *     extension. The file is overwritten if it already exists.
     * @returns {boolean} <code>true</code> if the specified file could be opened and audio recording has started, otherwise 
     *     <code>false</code>.
     * @example <caption>Make a 10 second audio recording.</caption>
     * var filename = File.getTempDir() + "/audio.wav";
     * if (Audio.startRecording(filename)) {
     *     Script.setTimeout(function () {
     *         Audio.stopRecording();
     *         print("Audio recording made in: " + filename);
     *     }, 10000);
     * 
     * } else {
     *     print("Could not make an audio recording in: " + filename);
     * }
     */
    Q_INVOKABLE bool startRecording(const QString& filename);
    
    /**jsdoc
     * Finish making an audio recording started with {@link Audio.startRecording|startRecording}.
     * @function Audio.stopRecording
     */
    Q_INVOKABLE void stopRecording();

    /**jsdoc
     * Check whether an audio recording is currently being made.
     * @function Audio.getRecording
     * @returns {boolean} <code>true</code> if an audio recording is currently being made, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool getRecording();

signals:

    /**jsdoc
     * @function Audio.nop
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void nop();

    /**jsdoc
     * Triggered when the audio input is muted or unmuted.
     * @function Audio.mutedChanged
     * @param {boolean} isMuted - <code>true</code> if the audio input is muted, otherwise <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when audio input is muted or unmuted</caption>
     * Audio.mutedChanged.connect(function (isMuted) {
     *     print("Audio muted: " + isMuted);
     * });
     */
    void mutedChanged(bool isMuted);
    
    /**jsdoc
    * Triggered when desktop audio input is muted or unmuted.
    * @function Audio.desktopMutedChanged
    * @param {boolean} isMuted - <code>true</code> if the audio input is muted for desktop mode, otherwise <code>false</code>.
    * @returns {Signal}
    */
    void desktopMutedChanged(bool isMuted);

    /**jsdoc
    * Triggered when HMD audio input is muted or unmuted.
    * @function Audio.hmdMutedChanged
    * @param {boolean} isMuted - <code>true</code> if the audio input is muted for HMD mode, otherwise <code>false</code>.
    * @returns {Signal}
    */
    void hmdMutedChanged(bool isMuted);

    /**
    * Triggered when Push-to-Talk has been enabled or disabled.
    * @function Audio.pushToTalkChanged
    * @param {boolean} enabled - <code>true</code> if Push-to-Talk is enabled, otherwise <code>false</code>.
    * @returns {Signal}
    */
    void pushToTalkChanged(bool enabled);

    /**
    * Triggered when Push-to-Talk has been enabled or disabled for desktop mode.
    * @function Audio.pushToTalkDesktopChanged
    * @param {boolean} enabled - <code>true</code> if Push-to-Talk is emabled for Desktop mode, otherwise <code>false</code>.
    * @returns {Signal}
    */
    void pushToTalkDesktopChanged(bool enabled);

    /**
    * Triggered when Push-to-Talk has been enabled or disabled for HMD mode.
    * @function Audio.pushToTalkHMDChanged
    * @param {boolean} enabled - <code>true</code> if Push-to-Talk is emabled for HMD mode, otherwise <code>false</code>.
    * @returns {Signal}
    */
    void pushToTalkHMDChanged(bool enabled);

    /**jsdoc
     * Triggered when the audio input noise reduction is enabled or disabled.
     * @function Audio.noiseReductionChanged
     * @param {boolean} isEnabled - <code>true</code> if audio input noise reduction is enabled, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void noiseReductionChanged(bool isEnabled);

    /**jsdoc
     * Triggered when the input audio volume changes.
     * @function Audio.inputVolumeChanged
     * @param {number} volume - The requested volume to be applied to the audio input, range <code>0.0</code> &ndash; 
     *     <code>1.0</code>. The resulting value of <code>Audio.inputVolume</code> depends on the capabilities of the device: 
     *     for example, the volume can't be changed on some devices, and others might only support values of <code>0.0</code> 
     *     and <code>1.0</code>.
     * @returns {Signal}
     */
    void inputVolumeChanged(float volume);

    /**jsdoc
     * Triggered when the input audio level changes.
     * @function Audio.inputLevelChanged
     * @param {number} level - The loudness of the input audio, range <code>0.0</code> (no sound) &ndash; <code>1.0</code> (the 
     *     onset of clipping).
     * @returns {Signal}
     */
    void inputLevelChanged(float level);

    /**jsdoc
     * Triggered when the clipping state of the input audio changes.
     * @function Audio.clippingChanged
     * @param {boolean} isClipping - <code>true</code> if the audio input is clipping, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void clippingChanged(bool isClipping);

    /**jsdoc
     * Triggered when the current context of the audio changes.
     * @function Audio.contextChanged
     * @param {string} context - The current context of the audio: either <code>"Desktop"</code> or <code>"HMD"</code>.
     * @returns {Signal}
     */
    void contextChanged(const QString& context);

    /**jsdoc
    * Triggered when pushing to talk.
    * @function Audio.pushingToTalkChanged
    * @param {boolean} talking - <code>true</code> if broadcasting with PTT, <code>false</code> otherwise.
    * @returns {Signal}
    */
    void pushingToTalkChanged(bool talking);

public slots:

    /**jsdoc
     * @function Audio.onContextChanged
     * @deprecated This function is deprecated and will be removed.
     */
    void onContextChanged();

    void handlePushedToTalk(bool enabled);

private slots:
    void setMuted(bool muted);
    void enableNoiseReduction(bool enable);
    void setInputVolume(float volume);
    void onInputLoudnessChanged(float loudness, bool isClipping);

protected:
    // Audio must live on a separate thread from AudioClient to avoid deadlocks
    Audio();

private:

    float _inputVolume { 1.0f };
    float _inputLevel { 0.0f };
    bool _isClipping { false };
    bool _enableNoiseReduction { true };  // Match default value of AudioClient::_isNoiseGateEnabled.
    bool _contextIsHMD { false };
    AudioDevices* getDevices() { return &_devices; }
    AudioDevices _devices;
    Setting::Handle<bool> _desktopMutedSetting{ QStringList { Audio::AUDIO, "desktopMuted" }, true };
    Setting::Handle<bool> _hmdMutedSetting{ QStringList { Audio::AUDIO, "hmdMuted" }, true };
    Setting::Handle<bool> _pttDesktopSetting{ QStringList { Audio::AUDIO, "pushToTalkDesktop" }, false };
    Setting::Handle<bool> _pttHMDSetting{ QStringList { Audio::AUDIO, "pushToTalkHMD" }, false };
    bool _desktopMuted{ true };
    bool _hmdMuted{ false };
    bool _pttDesktop{ false };
    bool _pttHMD{ false };
    bool _pushingToTalk{ false };
};

};

#endif // hifi_scripting_Audio_h
