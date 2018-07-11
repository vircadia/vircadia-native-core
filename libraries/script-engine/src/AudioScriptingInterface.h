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
     * @function Audio.playSound
     * @param {} sound
     * @param {} [injectorOptions=null]
     * @returns {object}
     */
    Q_INVOKABLE ScriptAudioInjector* playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions = AudioInjectorOptions());

    /**jsdoc
     * @function Audio.playSystemSound
     * @param {} sound
     * @param {} position
     * @returns {object}
     */
    // FIXME: there is no way to play a positionless sound
    Q_INVOKABLE ScriptAudioInjector* playSystemSound(SharedSoundPointer sound, const QVector3D& position);

    /**jsdoc
     * @function Audio.setStereoInput
     * @param {boolean} stereo
     */
    Q_INVOKABLE void setStereoInput(bool stereo);

    /**jsdoc
     * @function Audio.isStereoInput
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isStereoInput();

signals:

    /**jsdoc
     * The client has been muted by the mixer.
     * @function Audio.mutedByMixer
     * @returns {Signal} 
     */
    void mutedByMixer();

    /**jsdoc
     * The entire environment has been muted by the mixer.
     * @function Audio.environmentMuted
     * @returns {Signal} 
     */
    void environmentMuted();

    /**jsdoc
     * The client has received its first packet from the audio mixer.
     * @function Audio.receivedFirstPacket
     * @returns {Signal} 
     */
    void receivedFirstPacket();

    /**jsdoc
     * The client has been disconnected from the audio mixer.
     * @function Audio.disconnected
     * @returns {Signal} 
     */
    void disconnected();

    /**jsdoc
     * The noise gate has opened.
     * @function Audio.noiseGateOpened
     * @returns {Signal} 
     */
    void noiseGateOpened();

    /**jsdoc
     * The noise gate has closed.
     * @function Audio.noiseGateClosed
     * @returns {Signal} 
     */
    void noiseGateClosed();

    /**jsdoc
     * A frame of mic input audio has been received and processed.
     * @function Audio.inputReceived
     * @param {} inputSamples
     * @returns {Signal} 
     */
    void inputReceived(const QByteArray& inputSamples);

    /**jsdoc
    * @function Audio.isStereoInputChanged
    * @param {boolean} isStereo
    * @returns {Signal}
    */
    void isStereoInputChanged(bool isStereo);

private:
    AbstractAudioInterface* _localAudioInterface { nullptr };
};

void registerAudioMetaTypes(QScriptEngine* engine);

#endif // hifi_AudioScriptingInterface_h
