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

#include <QVector3D>

#include <shared/QtHelpers.h>

#include "ScriptAudioInjector.h"
#include "ScriptEngineLogging.h"

void registerAudioMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, injectorOptionsToScriptValue, injectorOptionsFromScriptValue);
    qScriptRegisterMetaType(engine, soundSharedPointerToScriptValue, soundSharedPointerFromScriptValue);
}

ScriptAudioInjector* AudioScriptingInterface::playSystemSound(SharedSoundPointer sound, const QVector3D& position) {
    AudioInjectorOptions options;
    options.position = glm::vec3(position.x(), position.y(), position.z());
    options.localOnly = true;
    return playSound(sound, options);
}

ScriptAudioInjector* AudioScriptingInterface::playSound(SharedSoundPointer sound, const AudioInjectorOptions& injectorOptions) {
    if (QThread::currentThread() != thread()) {
        ScriptAudioInjector* injector = NULL;

        BLOCKING_INVOKE_METHOD(this, "playSound",
                                  Q_RETURN_ARG(ScriptAudioInjector*, injector),
                                  Q_ARG(SharedSoundPointer, sound),
                                  Q_ARG(const AudioInjectorOptions&, injectorOptions));
        return injector;
    }

    if (sound) {
        // stereo option isn't set from script, this comes from sound metadata or filename
        AudioInjectorOptions optionsCopy = injectorOptions;
        optionsCopy.stereo = sound->isStereo();
        optionsCopy.ambisonic = sound->isAmbisonic();
        optionsCopy.localOnly = optionsCopy.localOnly || sound->isAmbisonic();  // force localOnly when Ambisonic

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
