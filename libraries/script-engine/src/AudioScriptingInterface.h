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
#include <Sound.h>

class ScriptAudioInjector;

class AudioScriptingInterface : public QObject {
    Q_OBJECT
public:
    static AudioScriptingInterface& getInstance();

    void setLocalAudioInterface(AbstractAudioInterface* audioInterface) { _localAudioInterface = audioInterface; }

protected:
    // this method is protected to stop C++ callers from calling, but invokable from script
    Q_INVOKABLE ScriptAudioInjector* playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions = AudioInjectorOptions());

    Q_INVOKABLE void setStereoInput(bool stereo);

signals:
    void mutedByMixer();
    void environmentMuted();
    void receivedFirstPacket();
    void disconnected();

private:
    AudioScriptingInterface();
    AbstractAudioInterface* _localAudioInterface;
};

void registerAudioMetaTypes(QScriptEngine* engine);

#endif // hifi_AudioScriptingInterface_h
