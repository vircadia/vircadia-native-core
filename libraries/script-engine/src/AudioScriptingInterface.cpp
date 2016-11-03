//
//  AudioScriptingInterface.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioScriptingInterface.h"

#include "ScriptAudioInjector.h"
#include "ScriptEngineLogging.h"

void registerAudioMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, injectorOptionsToScriptValue, injectorOptionsFromScriptValue);
    qScriptRegisterMetaType(engine, soundSharedPointerToScriptValue, soundSharedPointerFromScriptValue);
}

AudioScriptingInterface& AudioScriptingInterface::getInstance() {
    static AudioScriptingInterface staticInstance;
    return staticInstance;
}

AudioScriptingInterface::AudioScriptingInterface() :
    _localAudioInterface(NULL)
{

}

ScriptAudioInjector* AudioScriptingInterface::playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions) {
    if (QThread::currentThread() != thread()) {
        ScriptAudioInjector* injector = NULL;

        QMetaObject::invokeMethod(this, "playSound", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(ScriptAudioInjector*, injector),
                                  Q_ARG(SharedSoundPointer, sound),
                                  Q_ARG(const AudioInjectorOptions&, injectorOptions));
        return injector;
    }

    if (sound) {
        // stereo option isn't set from script, this comes from sound metadata or filename
        AudioInjectorOptions optionsCopy = injectorOptions;
        optionsCopy.stereo = sound->isStereo();
        auto injector = AudioInjector::playSound(sound->getByteArray(), optionsCopy);
        if (!injector) {
            return NULL;
        }
        return new ScriptAudioInjector(injector);

    } else {
        qCDebug(scriptengine) << "AudioScriptingInterface::playSound called with null Sound object.";
        return NULL;
    }
}

void AudioScriptingInterface::setStereoInput(bool stereo) {
    if (_localAudioInterface) {
        _localAudioInterface->setIsStereoInput(stereo);
    }
}
