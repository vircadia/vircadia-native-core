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
#include <HifiAudioDeviceInfo.h>

using MutedGetter = std::function<bool()>;
using MutedSetter = std::function<void(bool)>;

namespace scripting {

class Audio : public AudioScriptingInterface, protected ReadWriteLockable {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    /*@jsdoc
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
     * @property {boolean} muted - <code>true</code> if the audio input is muted for the current user context (desktop or HMD), 
     *     otherwise <code>false</code>.
     * @property {boolean} mutedDesktop - <code>true</code> if desktop audio input is muted, otherwise <code>false</code>.
     * @property {boolean} mutedHMD - <code>true</code> if the HMD input is muted, otherwise <code>false</code>.
     * @property {boolean} warnWhenMuted - <code>true</code> if the "muted" warning is enabled, otherwise <code>false</code>.
     *     When enabled, if you speak while your microphone is muted, "muted" is displayed on the screen as a warning.
     * @property {boolean} noiseReduction - <code>true</code> if noise reduction is enabled, otherwise <code>false</code>. When
     *     enabled, the input audio signal is blocked (fully attenuated) when it falls below an adaptive threshold set just
     *     above the noise floor.
     * @property {boolean} noiseReductionAutomatic - <code>true</code> if audio input noise reduction automatic mode is enabled, 
     *     <code>false</code> if in manual mode. Manual mode allows you to use <code>Audio.noiseReductionThreshold</code>
     *     to set a manual sensitivity for the noise gate.
     * @property {number} noiseReductionThreshold - Sets the noise gate threshold before your mic audio is transmitted. 
     *     (Applies only if <code>Audio.noiseReductionAutomatic</code> is <code>false</code>.)
     * @property {number} inputVolume - Adjusts the volume of the input audio, range <code>0.0</code> &ndash; <code>1.0</code>. 
     *     If set to a value, the resulting value depends on the input device: for example, the volume can't be changed on some 
     *     devices, and others might only support values of <code>0.0</code> and <code>1.0</code>.
     * @property {number} inputLevel - The loudness of the audio input, range <code>0.0</code> (no sound) &ndash;
     *     <code>1.0</code> (the onset of clipping). <em>Read-only.</em>
     * @property {boolean} clipping - <code>true</code> if the audio input is clipping, otherwise <code>false</code>.
     * @property {string} context - The current context of the audio: either <code>"Desktop"</code> or <code>"HMD"</code>.
     *     <em>Read-only.</em>
     * @property {object} devices - <em>Read-only.</em>
     *     <p class="important">Deprecated: This property is deprecated and will be removed.
     * @property {boolean} pushToTalk - <code>true</code> if push-to-talk is enabled for the current user context (desktop or 
     *     HMD), otherwise <code>false</code>.
     * @property {boolean} pushToTalkDesktop - <code>true</code> if desktop push-to-talk is enabled, otherwise 
     *     <code>false</code>.
     * @property {boolean} pushToTalkHMD - <code>true</code> if HMD push-to-talk is enabled, otherwise <code>false</code>.
     * @property {boolean} pushingToTalk - <code>true</code> if the user is currently pushing-to-talk, otherwise 
     *     <code>false</code>.
     
     * @property {number} avatarGain - The gain (relative volume in dB) that avatars' voices are played at. This gain is used 
     *     at the server.
     * @property {number} localInjectorGain - The gain (relative volume in dB) that local injectors (local environment sounds) 
     *    are played at.
     * @property {number} serverInjectorGain - The gain (relative volume in dB) that server injectors (server environment 
     *     sounds) are played at. This gain is used at the server.
     * @property {number} systemInjectorGain - The gain (relative volume in dB) that system sounds are played at.
     * @property {number} pushingToTalkOutputGainDesktop - The gain (relative volume in dB) that all sounds are played at when 
     *     the user is holding the push-to-talk key in desktop mode.
     * @property {boolean} acousticEchoCancellation - <code>true</code> if acoustic echo cancellation is enabled, otherwise
     *     <code>false</code>. When enabled, sound from the audio output is suppressed when it echos back to the input audio 
     *     signal.
     *
     * @comment The following properties are from AudioScriptingInterface.h.
     * @property {boolean} isStereoInput - <code>true</code> if the input audio is being used in stereo, otherwise
     *     <code>false</code>. Some devices do not support stereo, in which case the value is always <code>false</code>.
     * @property {boolean} isSoloing - <code>true</code> if currently audio soloing, i.e., playing audio from only specific 
     *     avatars. <em>Read-only.</em>
     * @property {Uuid[]} soloList - The list of currently soloed avatar IDs. Empty list if not currently audio soloing. 
     *     <em>Read-only.</em>
     */

    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool noiseReduction READ noiseReductionEnabled WRITE enableNoiseReduction NOTIFY noiseReductionChanged)
    Q_PROPERTY(bool noiseReductionAutomatic READ noiseReductionAutomatic WRITE enableNoiseReductionAutomatic NOTIFY noiseReductionAutomaticChanged)
    Q_PROPERTY(float noiseReductionThreshold READ getNoiseReductionThreshold WRITE setNoiseReductionThreshold NOTIFY noiseReductionThresholdChanged)
    Q_PROPERTY(bool warnWhenMuted READ warnWhenMutedEnabled WRITE enableWarnWhenMuted NOTIFY warnWhenMutedChanged)
    Q_PROPERTY(bool acousticEchoCancellation
               READ acousticEchoCancellationEnabled WRITE enableAcousticEchoCancellation NOTIFY acousticEchoCancellationChanged)
    Q_PROPERTY(float inputVolume READ getInputVolume WRITE setInputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(float inputLevel READ getInputLevel NOTIFY inputLevelChanged)
    Q_PROPERTY(bool clipping READ isClipping NOTIFY clippingChanged)
    Q_PROPERTY(QString context READ getContext NOTIFY contextChanged)
    Q_PROPERTY(AudioDevices* devices READ getDevices NOTIFY nop)
    Q_PROPERTY(bool mutedDesktop READ getMutedDesktop WRITE setMutedDesktop NOTIFY mutedDesktopChanged)
    Q_PROPERTY(bool mutedHMD READ getMutedHMD WRITE setMutedHMD NOTIFY mutedHMDChanged)
    Q_PROPERTY(bool pushToTalk READ getPTT WRITE setPTT NOTIFY pushToTalkChanged);
    Q_PROPERTY(bool pushToTalkDesktop READ getPTTDesktop WRITE setPTTDesktop NOTIFY pushToTalkDesktopChanged)
    Q_PROPERTY(bool pushToTalkHMD READ getPTTHMD WRITE setPTTHMD NOTIFY pushToTalkHMDChanged)
    Q_PROPERTY(bool pushingToTalk READ getPushingToTalk WRITE setPushingToTalk NOTIFY pushingToTalkChanged)
    Q_PROPERTY(float pushingToTalkOutputGainDesktop READ getPushingToTalkOutputGainDesktop
        WRITE setPushingToTalkOutputGainDesktop NOTIFY pushingToTalkOutputGainDesktopChanged)
    Q_PROPERTY(float avatarGain READ getAvatarGain WRITE setAvatarGain NOTIFY avatarGainChanged)
    Q_PROPERTY(float localInjectorGain READ getLocalInjectorGain WRITE setLocalInjectorGain NOTIFY localInjectorGainChanged)
    Q_PROPERTY(float serverInjectorGain READ getInjectorGain WRITE setInjectorGain NOTIFY serverInjectorGainChanged)
    Q_PROPERTY(float systemInjectorGain READ getSystemInjectorGain WRITE setSystemInjectorGain NOTIFY systemInjectorGainChanged)

public:
    static QString AUDIO;
    static QString HMD;
    static QString DESKTOP;

    static float loudnessToLevel(float loudness);

    virtual ~Audio() {}

    bool isMuted() const;
    bool noiseReductionEnabled() const;
    bool noiseReductionAutomatic() const;
    bool warnWhenMutedEnabled() const;
    bool acousticEchoCancellationEnabled() const;
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

    /*@jsdoc
     * @function Audio.setInputDevice
     * @param {object} device - Device.
     * @param {boolean} isHMD - Is HMD.
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE void setInputDevice(const HifiAudioDeviceInfo& device, bool isHMD);

    /*@jsdoc
     * @function Audio.setOutputDevice
     * @param {object} device - Device.
     * @param {boolean} isHMD - Is HMD.
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE void setOutputDevice(const HifiAudioDeviceInfo& device, bool isHMD);

    /*@jsdoc
     * Enables or disables reverberation. Reverberation is done by the client on the post-mix audio. The reverberation options
     * come from either the domain's audio zone configured on the server or settings scripted by
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

    /*@jsdoc
     * Configures reverberation options. Use {@link Audio.setReverb|setReverb} to enable or disable reverberation.
     * @function Audio.setReverbOptions
     * @param {AudioEffectOptions} options - The reverberation options.
     */
    Q_INVOKABLE void setReverbOptions(const AudioEffectOptions* options);

    /*@jsdoc
     * Sets the gain (relative volume) that avatars' voices are played at. This gain is used at the server.
     * @function Audio.setAvatarGain
     * @param {number} gain - The avatar gain (dB) at the server.
     */
    Q_INVOKABLE void setAvatarGain(float gain);

    /*@jsdoc
     * Gets the gain (relative volume) that avatars' voices are played at. This gain is used at the server.
     * @function Audio.getAvatarGain
     * @returns {number} The avatar gain (dB) at the server.
     * @example <caption>Report current audio gain settings.</caption>
     * // 0 value = normal volume; -ve value = quieter; +ve value = louder.
     * print("Avatar gain: " + Audio.getAvatarGain());
     * print("Environment server gain: " + Audio.getInjectorGain());
     * print("Environment local gain: " + Audio.getLocalInjectorGain());
     * print("System gain: " + Audio.getSystemInjectorGain());
     */
    Q_INVOKABLE float getAvatarGain();

    /*@jsdoc
     * Sets the gain (relative volume) that environment sounds from the server are played at.
     * @function Audio.setInjectorGain
     * @param {number} gain - The injector gain (dB) at the server.
     */
    Q_INVOKABLE void setInjectorGain(float gain);

    /*@jsdoc
     * Gets the gain (relative volume) that environment sounds from the server are played at.
     * @function Audio.getInjectorGain
     * @returns {number} The injector gain (dB) at the server.
     */
    Q_INVOKABLE float getInjectorGain();

    /*@jsdoc
     * Sets the gain (relative volume) that environment sounds from the client are played at.
     * @function Audio.setLocalInjectorGain
     * @param {number} gain - The injector gain (dB) in the client.
     */
    Q_INVOKABLE void setLocalInjectorGain(float gain);

    /*@jsdoc
     * Gets the gain (relative volume) that environment sounds from the client are played at.
     * @function Audio.getLocalInjectorGain
     * @returns {number} The injector gain (dB) in the client.
     */
    Q_INVOKABLE float getLocalInjectorGain();

    /*@jsdoc
     * Sets the gain (relative volume) that system sounds are played at.
     * @function Audio.setSystemInjectorGain
     * @param {number} gain - The injector gain (dB) in the client.
     */
    Q_INVOKABLE void setSystemInjectorGain(float gain);

    /*@jsdoc
     * Gets the gain (relative volume) that system sounds are played at.
     * @function Audio.getSystemInjectorGain
     * @returns {number} The injector gain (dB) in the client.
    */
    Q_INVOKABLE float getSystemInjectorGain();
    
    /*@jsdoc
     * Sets the noise gate threshold before your mic audio is transmitted. (Applies only if <code>Audio.noiseReductionAutomatic</code> is <code>false</code>.)
     * @function Audio.setNoiseReductionThreshold
     * @param {number} threshold - The level that your input must surpass to be transmitted. <code>0.0</code> (open the gate completely) &ndash; <code>1.0</code>
     */
    Q_INVOKABLE void setNoiseReductionThreshold(float threshold);

    /*@jsdoc
     * Gets the noise reduction threshold.
     * @function Audio.getNoiseReductionThreshold
     * @returns {number} The noise reduction threshold. <code>0.0</code> &ndash; <code>1.0</code>
    */
    Q_INVOKABLE float getNoiseReductionThreshold();

    /*@jsdoc
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

    /*@jsdoc
     * Finishes making an audio recording started with {@link Audio.startRecording|startRecording}.
     * @function Audio.stopRecording
     */
    Q_INVOKABLE void stopRecording();

    /*@jsdoc
     * Checks whether an audio recording is currently being made.
     * @function Audio.getRecording
     * @returns {boolean} <code>true</code> if an audio recording is currently being made, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool getRecording();

    /*@jsdoc
     * Sets the output volume gain that will be used when the user is holding the push-to-talk key.
     * Should be negative.
     * @function Audio.setPushingToTalkOutputGainDesktop
     * @param {number} gain - The output volume gain (dB) while using push-to-talk.
     */
    Q_INVOKABLE void setPushingToTalkOutputGainDesktop(float gain);

    /*@jsdoc
     * Gets the output volume gain that is used when the user is holding the push-to-talk key.
     * Should be negative.
     * @function Audio.getPushingToTalkOutputGainDesktop
     * @returns {number} gain - The output volume gain (dB) while using push-to-talk.
     */
    Q_INVOKABLE float getPushingToTalkOutputGainDesktop();

signals:

    /*@jsdoc
     * @function Audio.nop
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void nop();

    /*@jsdoc
     * Triggered when the audio input is muted or unmuted for the current context (desktop or HMD).
     * @function Audio.mutedChanged
     * @param {boolean} isMuted - <code>true</code> if the audio input is muted for the current context (desktop or HMD), 
     *     otherwise <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when audio input is muted or unmuted</caption>
     * Audio.mutedChanged.connect(function (isMuted) {
     *     print("Audio muted: " + isMuted);
     * });
     */
    void mutedChanged(bool isMuted);

    /*@jsdoc
     * Triggered when desktop audio input is muted or unmuted.
     * @function Audio.mutedDesektopChanged
     * @param {boolean} isMuted - <code>true</code> if desktop audio input is muted, otherwise <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when desktop muting changes.</caption>
     * Audio.mutedDesktopChanged.connect(function (isMuted) {
     *     print("Desktop muted: " + isMuted);
     * });
     */
    void mutedDesktopChanged(bool isMuted);

    /*@jsdoc
     * Triggered when HMD audio input is muted or unmuted.
     * @function Audio.mutedHMDChanged
     * @param {boolean} isMuted - <code>true</code> if HMD audio input is muted, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void mutedHMDChanged(bool isMuted);

    /*@jsdoc
     * Triggered when push-to-talk is enabled or disabled for the current context (desktop or HMD).
     * @function Audio.pushToTalkChanged
     * @param {boolean} enabled - <code>true</code> if push-to-talk is enabled, otherwise <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when push-to-talk changes.</caption>
     * Audio.pushToTalkChanged.connect(function (enabled) {
     *     print("Push to talk: " + (enabled ? "on" : "off"));
     * });
     */
    void pushToTalkChanged(bool enabled);

    /*@jsdoc
     * Triggered when push-to-talk is enabled or disabled for desktop mode.
     * @function Audio.pushToTalkDesktopChanged
     * @param {boolean} enabled - <code>true</code> if push-to-talk is enabled for desktop mode, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void pushToTalkDesktopChanged(bool enabled);

    /*@jsdoc
     * Triggered when push-to-talk is enabled or disabled for HMD mode.
     * @function Audio.pushToTalkHMDChanged
     * @param {boolean} enabled - <code>true</code> if push-to-talk is enabled for HMD mode, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void pushToTalkHMDChanged(bool enabled);

    /*@jsdoc
     * Triggered when audio input noise reduction is enabled or disabled.
     * @function Audio.noiseReductionChanged
     * @param {boolean} isEnabled - <code>true</code> if audio input noise reduction is enabled, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void noiseReductionChanged(bool isEnabled);
    
    /*@jsdoc
     * Triggered when the audio input noise reduction mode is changed.
     * @function Audio.noiseReductionAutomaticChanged
     * @param {boolean} isEnabled - <code>true</code> if audio input noise reduction automatic mode is enabled, <code>false</code> if in manual mode.
     * @returns {Signal}
     */
    void noiseReductionAutomaticChanged(bool isEnabled);
    
    /*@jsdoc
     * Triggered when the audio input noise reduction threshold is changed.
     * @function Audio.noiseReductionThresholdChanged
     * @param {number} threshold - The threshold for the audio input noise reduction, range <code>0.0</code> (open the gate completely) &ndash; <code>1.0</code>
     *     (close the gate completely).
     * @returns {Signal}
     */
    void noiseReductionThresholdChanged(float threshold);

    /*@jsdoc
     * Triggered when "warn when muted" is enabled or disabled.
     * @function Audio.warnWhenMutedChanged
     * @param {boolean} isEnabled - <code>true</code> if "warn when muted" is enabled, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void warnWhenMutedChanged(bool isEnabled);

    /*@jsdoc
     * Triggered when acoustic echo cancellation is enabled or disabled.
     * @function Audio.acousticEchoCancellationChanged
     * @param {boolean} isEnabled - <code>true</code> if acoustic echo cancellation is enabled, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void acousticEchoCancellationChanged(bool isEnabled);

    /*@jsdoc
     * Triggered when the input audio volume changes.
     * @function Audio.inputVolumeChanged
     * @param {number} volume - The requested volume to be applied to the audio input, range <code>0.0</code> &ndash;
     *     <code>1.0</code>. The resulting value of <code>Audio.inputVolume</code> depends on the capabilities of the device.
     *     For example, the volume can't be changed on some devices, while others might only support values of <code>0.0</code>
     *     and <code>1.0</code>.
     * @returns {Signal}
     */
    void inputVolumeChanged(float volume);

    /*@jsdoc
     * Triggered when the input audio level changes.
     * @function Audio.inputLevelChanged
     * @param {number} level - The loudness of the input audio, range <code>0.0</code> (no sound) &ndash; <code>1.0</code> (the
     *     onset of clipping).
     * @returns {Signal}
     */
    void inputLevelChanged(float level);

    /*@jsdoc
     * Triggered when the clipping state of the input audio changes.
     * @function Audio.clippingChanged
     * @param {boolean} isClipping - <code>true</code> if the audio input is clipping, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void clippingChanged(bool isClipping);

    /*@jsdoc
     * Triggered when the current context of the audio changes.
     * @function Audio.contextChanged
     * @param {string} context - The current context of the audio: either <code>"Desktop"</code> or <code>"HMD"</code>.
     * @returns {Signal}
     */
    void contextChanged(const QString& context);

    /*@jsdoc
     * Triggered when the user starts or stops push-to-talk.
     * @function Audio.pushingToTalkChanged
     * @param {boolean} talking - <code>true</code> if started push-to-talk, <code>false</code> if stopped push-to-talk.
     * @returns {Signal}
     */
    void pushingToTalkChanged(bool talking);

    /*@jsdoc
     * Triggered when the avatar gain changes.
     * @function Audio.avatarGainChanged
     * @param {number} gain - The new avatar gain value (dB).
     * @returns {Signal}
     */
    void avatarGainChanged(float gain);

    /*@jsdoc
     * Triggered when the local injector gain changes.
     * @function Audio.localInjectorGainChanged
     * @param {number} gain - The new local injector gain value (dB).
     * @returns {Signal}
     */
    void localInjectorGainChanged(float gain);

    /*@jsdoc
     * Triggered when the server injector gain changes.
     * @function Audio.serverInjectorGainChanged
     * @param {number} gain - The new server injector gain value (dB).
     * @returns {Signal}
     */
    void serverInjectorGainChanged(float gain);

    /*@jsdoc
     * Triggered when the system injector gain changes.
     * @function Audio.systemInjectorGainChanged
     * @param {number} gain - The new system injector gain value (dB).
     * @returns {Signal}
     */
    void systemInjectorGainChanged(float gain);

    /*@jsdoc
     * Triggered when the push to talk gain changes.
     * @function Audio.pushingToTalkOutputGainDesktopChanged
     * @param {number} gain - The new output gain value (dB).
     * @returns {Signal}
     */
    void pushingToTalkOutputGainDesktopChanged(float gain);

public slots:

    /*@jsdoc
     * @function Audio.onContextChanged
     * @deprecated This function is deprecated and will be removed.
     */
    void onContextChanged();

    void handlePushedToTalk(bool enabled);

private slots:
    void setMuted(bool muted);
    void enableNoiseReduction(bool enable);
    void enableNoiseReductionAutomatic(bool enable);
    void enableWarnWhenMuted(bool enable);
    void enableAcousticEchoCancellation(bool enable);
    void setInputVolume(float volume);
    void onInputLoudnessChanged(float loudness, bool isClipping);

protected:
    // Audio must live on a separate thread from AudioClient to avoid deadlocks
    Audio();

private:

    bool _settingsLoaded { false };
    float _inputVolume { 1.0f };
    float _inputLevel { 0.0f };
    Setting::Handle<float> _avatarGainSetting { QStringList { Audio::AUDIO, "AvatarGain" }, 0.0f };
    Setting::Handle<float> _injectorGainSetting { QStringList { Audio::AUDIO, "InjectorGain" }, 0.0f };
    Setting::Handle<float> _localInjectorGainSetting { QStringList { Audio::AUDIO, "LocalInjectorGain" }, 0.0f };
    Setting::Handle<float> _systemInjectorGainSetting { QStringList { Audio::AUDIO, "SystemInjectorGain" }, 0.0f };
    float _localInjectorGain { 0.0f };      // in dB
    float _systemInjectorGain { 0.0f };     // in dB
    float _pttOutputGainDesktop { 0.0f };   // in dB
    float _noiseReductionThreshold { 0.1f }; // Match default value of AudioClient::_noiseReductionThreshold.
    bool _isClipping { false };
    bool _enableNoiseReduction { true };  // Match default value of AudioClient::_isNoiseGateEnabled.
    bool _noiseReductionAutomatic { true }; // Match default value of AudioClient::_noiseReductionAutomatic.
    bool _enableWarnWhenMuted { true };
    bool _enableAcousticEchoCancellation { true }; // AudioClient::_isAECEnabled
    bool _contextIsHMD { false };
    AudioDevices* getDevices() { return &_devices; }
    AudioDevices _devices;
    Setting::Handle<bool> _mutedDesktopSetting{ QStringList { Audio::AUDIO, "mutedDesktop" }, true };
    Setting::Handle<bool> _mutedHMDSetting{ QStringList { Audio::AUDIO, "mutedHMD" }, true };
    Setting::Handle<bool> _pttDesktopSetting{ QStringList { Audio::AUDIO, "pushToTalkDesktop" }, false };
    Setting::Handle<bool> _pttHMDSetting{ QStringList { Audio::AUDIO, "pushToTalkHMD" }, false };
    bool _mutedDesktop{ true };
    bool _mutedHMD{ false };
    bool _pttDesktop{ false };
    bool _pttHMD{ false };
    bool _pushingToTalk{ false };
};

};

#endif // hifi_scripting_Audio_h
