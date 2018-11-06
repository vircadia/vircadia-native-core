//
//  AudioScriptingInterface.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioScriptingInterface_h
#define hifi_AudioScriptingInterface_h

#include <AbstractAudioInterface.h>
#include <AudioInjector.h>
#include <DependencyManager.h>
#include <Sound.h>

class ScriptAudioInjector;

class AudioScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    // JSDoc for property is in Audio.h.
    Q_PROPERTY(bool isStereoInput READ isStereoInput WRITE setStereoInput NOTIFY isStereoInputChanged)
    Q_PROPERTY(bool isSoloing READ isSoloing)
    Q_PROPERTY(QVector<QUuid> soloList READ getSoloList)

public:
    virtual ~AudioScriptingInterface() = default;
    void setLocalAudioInterface(AbstractAudioInterface* audioInterface);

    bool isSoloing() const {
        return _localAudioInterface->getAudioSolo().isSoloing();
    }

    QVector<QUuid> getSoloList() const {
        return _localAudioInterface->getAudioSolo().getUUIDs();
    }

    /**jsdoc
     * Add nodes to the audio solo list
     * @function Audio.addToSoloList
     * @param {Uuid[]} uuidList - List of node UUIDs to add to the solo list.
     */
    Q_INVOKABLE void addToSoloList(QVector<QUuid> uuidList) {
        _localAudioInterface->getAudioSolo().addUUIDs(uuidList);
    }

    /**jsdoc
     * Remove nodes from the audio solo list
     * @function Audio.removeFromSoloList
     * @param {Uuid[]} uuidList - List of node UUIDs to remove from the solo list.
     */
    Q_INVOKABLE void removeFromSoloList(QVector<QUuid> uuidList) {
        _localAudioInterface->getAudioSolo().removeUUIDs(uuidList);
    }

    /**jsdoc
     * Reset the list of soloed nodes.
     * @function Audio.resetSoloList
     */
    Q_INVOKABLE void resetSoloList() {
        _localAudioInterface->getAudioSolo().reset();
    }

protected:
    AudioScriptingInterface() = default;

    // these methods are protected to stop C++ callers from calling, but invokable from script

    /**jsdoc
     * Starts playing &mdash; "injecting" &mdash; the content of an audio file. The sound is played globally (sent to the audio 
     * mixer) so that everyone hears it, unless the <code>injectorOptions</code> has <code>localOnly</code> set to 
     * <code>true</code> in which case only the client hears the sound played. No sound is played if sent to the audio mixer 
     * but the client is not connected to an audio mixer. The {@link AudioInjector} object returned by the function can be used 
     * to control the playback and get information about its current state.
     * @function Audio.playSound
     * @param {SoundObject} sound - The content of an audio file, loaded using {@link SoundCache.getSound}. See 
     * {@link SoundObject} for supported formats.
     * @param {AudioInjector.AudioInjectorOptions} [injectorOptions={}] - Audio injector configuration.
     * @returns {AudioInjector} The audio injector that plays the audio file.
     * @example <caption>Play a sound.</caption>
     * var sound = SoundCache.getSound(Script.resourcesPath() + "sounds/sample.wav");
     * var injector;
     * var injectorOptions = {
     *     position: MyAvatar.position
     * };
     * 
     * Script.setTimeout(function () { // Give the sound time to load.
     *     injector = Audio.playSound(sound, injectorOptions);
     * }, 1000);
     */
    Q_INVOKABLE ScriptAudioInjector* playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions = AudioInjectorOptions());

    /**jsdoc
     * Start playing the content of an audio file, locally (isn't sent to the audio mixer). This is the same as calling 
     * {@link Audio.playSound} with {@link AudioInjector.AudioInjectorOptions} <code>localOnly</code> set <code>true</code> and 
     * the specified <code>position</code>.
     * @function Audio.playSystemSound
     * @param {SoundObject} sound - The content of an audio file, loaded using {@link SoundCache.getSound}. See 
     * {@link SoundObject} for supported formats.
     * @param {Vec3} position - The position in the domain to play the sound.
     * @returns {AudioInjector} The audio injector that plays the audio file.
     */
    // FIXME: there is no way to play a positionless sound
    Q_INVOKABLE ScriptAudioInjector* playSystemSound(SharedSoundPointer sound, const QVector3D& position);

    /**jsdoc
     * Set whether or not the audio input should be used in stereo. If the audio input does not support stereo then setting a 
     * value of <code>true</code> has no effect.
     * @function Audio.setStereoInput
     * @param {boolean} stereo - <code>true</code> if the audio input should be used in stereo, otherwise <code>false</code>.
     */
    Q_INVOKABLE void setStereoInput(bool stereo);

    /**jsdoc
     * Get whether or not the audio input is used in stereo.
     * @function Audio.isStereoInput
     * @returns {boolean} <code>true</code> if the audio input is used in stereo, otherwise <code>false</code>. 
     */
    Q_INVOKABLE bool isStereoInput();

signals:

    /**jsdoc
     * Triggered when the client is muted by the mixer because their loudness value for the noise background has reached the 
     * threshold set for the domain in the server settings.
     * @function Audio.mutedByMixer
     * @returns {Signal} 
     */
    void mutedByMixer();

    /**jsdoc
     * Triggered when the client is muted by the mixer because they're within a certain radius (50m) of someone who requested 
     * the mute through Developer &gt; Audio &gt; Mute Environment.
     * @function Audio.environmentMuted
     * @returns {Signal} 
     */
    void environmentMuted();

    /**jsdoc
     * Triggered when the client receives its first packet from the audio mixer.
     * @function Audio.receivedFirstPacket
     * @returns {Signal} 
     */
    void receivedFirstPacket();

    /**jsdoc
     * Triggered when the client is disconnected from the audio mixer.
     * @function Audio.disconnected
     * @returns {Signal} 
     */
    void disconnected();

    /**jsdoc
     * Triggered when the noise gate is opened: the input audio signal is no longer blocked (fully attenuated) because it has 
     * risen above an adaptive threshold set just above the noise floor. Only occurs if <code>Audio.noiseReduction</code> is 
     * <code>true</code>.
     * @function Audio.noiseGateOpened
     * @returns {Signal} 
     */
    void noiseGateOpened();

    /**jsdoc
     * Triggered when the noise gate is closed: the input audio signal is blocked (fully attenuated) because it has fallen 
     * below an adaptive threshold set just above the noise floor. Only occurs if <code>Audio.noiseReduction</code> is 
     * <code>true</code>.
     * @function Audio.noiseGateClosed
     * @returns {Signal} 
     */
    void noiseGateClosed();

    /**jsdoc
     * Triggered when a frame of audio input is processed.
     * @function Audio.inputReceived
     * @param {Int16Array} inputSamples - The audio input processed.
     * @returns {Signal} 
     */
    void inputReceived(const QByteArray& inputSamples);

    /**jsdoc
     * Triggered when the input audio use changes between mono and stereo.
     * @function Audio.isStereoInputChanged
     * @param {boolean} isStereo - <code>true</code> if the input audio is stereo, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void isStereoInputChanged(bool isStereo);

private:
    AbstractAudioInterface* _localAudioInterface { nullptr };
};

void registerAudioMetaTypes(QScriptEngine* engine);

#endif // hifi_AudioScriptingInterface_h
