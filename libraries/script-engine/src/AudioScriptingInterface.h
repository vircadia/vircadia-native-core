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

    Q_PROPERTY(bool isStereoInput READ isStereoInput WRITE setStereoInput NOTIFY isStereoInputChanged)

public:
    virtual ~AudioScriptingInterface() {}
    void setLocalAudioInterface(AbstractAudioInterface* audioInterface);

protected:
    AudioScriptingInterface() {}

    // these methods are protected to stop C++ callers from calling, but invokable from script

    /**jsdoc
     * TODO
     * @function Audio.playSound
     * @param {SoundObject} sound - TODO
     * @param {AudioInjector.AudioInjectorOptions} [injectorOptions={}] - TODO
     * @returns {AudioInjector} TODO
     */
    Q_INVOKABLE ScriptAudioInjector* playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions = AudioInjectorOptions());

    /**jsdoc
     * TODO
     * @function Audio.playSystemSound
     * @param {SoundObject} sound - TODO
     * @param {Vec3} position - TODO
     * @returns {AudioInjector} TODO
     */
    // FIXME: there is no way to play a positionless sound
    Q_INVOKABLE ScriptAudioInjector* playSystemSound(SharedSoundPointer sound, const QVector3D& position);

    /**jsdoc
     * TODO
     * @function Audio.setStereoInput
     * @param {boolean} stereo - TODO
     */
    Q_INVOKABLE void setStereoInput(bool stereo);

    /**jsdoc
     * TODO
     * @function Audio.isStereoInput
     * @returns {boolean} <code>true</code> if TODO, otherwise <code>false</code>. 
     */
    Q_INVOKABLE bool isStereoInput();

signals:

    /**jsdoc
     * Triggered when the client is muted by the mixer.
     * @function Audio.mutedByMixer
     * @returns {Signal} 
     */
    void mutedByMixer();

    /**jsdoc
     * Triggered when the entire environment is muted by the mixer. TODO: What is the "whole environment"?
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
     * Triggered when the noise gate is opened. TODO: Description of noise gate.
     * @function Audio.noiseGateOpened
     * @returns {Signal} 
     */
    void noiseGateOpened();

    /**jsdoc
     * Triggered when the noise gate is closed. TODO: Description of noise gate.
     * @function Audio.noiseGateClosed
     * @returns {Signal} 
     */
    void noiseGateClosed();

    /**jsdoc
     * Triggered when a frame of microphone audio input is processed.
     * @function Audio.inputReceived
     * @param {Int16Array} inputSamples - TODO
     * @returns {Signal} 
     */
    void inputReceived(const QByteArray& inputSamples);

    /**jsdoc
     * TODO
     * @function Audio.isStereoInputChanged
     * @param {boolean} isStereo - TODO
     * @returns {Signal}
     */
    void isStereoInputChanged(bool isStereo);

private:
    AbstractAudioInterface* _localAudioInterface { nullptr };
};

void registerAudioMetaTypes(QScriptEngine* engine);

#endif // hifi_AudioScriptingInterface_h
