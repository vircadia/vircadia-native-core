//
//  AudioInjectorOptions.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioInjectorOptions_h
#define hifi_AudioInjectorOptions_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <ScriptValue.h>

class ScriptEngine;

class AudioInjectorOptions {
public:
    AudioInjectorOptions();
    glm::vec3 position;
    bool positionSet;
    float volume;
    bool loop;
    glm::quat orientation;
    bool stereo;
    bool ambisonic;
    bool ignorePenumbra;
    bool localOnly;
    float secondOffset;
    float pitch;    // multiplier, where 2.0f shifts up one octave
};

Q_DECLARE_METATYPE(AudioInjectorOptions);

ScriptValue injectorOptionsToScriptValue(ScriptEngine* engine, const AudioInjectorOptions& injectorOptions);
bool injectorOptionsFromScriptValue(const ScriptValue& object, AudioInjectorOptions& injectorOptions);

#endif // hifi_AudioInjectorOptions_h
