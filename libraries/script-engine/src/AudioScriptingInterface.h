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

public:
    virtual ~AudioScriptingInterface() {}
    void setLocalAudioInterface(AbstractAudioInterface* audioInterface) { _localAudioInterface = audioInterface; }

protected:
    AudioScriptingInterface() {}

    // these methods are protected to stop C++ callers from calling, but invokable from script
    Q_INVOKABLE ScriptAudioInjector* playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions = AudioInjectorOptions());
    // FIXME: there is no way to play a positionless sound
    Q_INVOKABLE ScriptAudioInjector* playSystemSound(SharedSoundPointer sound, const QVector3D& position);

    Q_INVOKABLE void setStereoInput(bool stereo);

signals:
    void mutedByMixer(); /// the client has been muted by the mixer
    void environmentMuted(); /// the entire environment has been muted by the mixer
    void receivedFirstPacket(); /// the client has received its first packet from the audio mixer
    void disconnected(); /// the client has been disconnected from the audio mixer
    void noiseGateOpened(); /// the noise gate has opened
    void noiseGateClosed(); /// the noise gate has closed
    void inputReceived(const QByteArray& inputSamples); /// a frame of mic input audio has been received and processed

private:
    AbstractAudioInterface* _localAudioInterface { nullptr };
};

void registerAudioMetaTypes(QScriptEngine* engine);

#endif // hifi_AudioScriptingInterface_h
