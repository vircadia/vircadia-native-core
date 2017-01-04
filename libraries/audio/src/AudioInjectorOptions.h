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

#include <QtScript/qscriptengine.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class AudioInjectorOptions {
public:
    AudioInjectorOptions();
    glm::vec3 position;
    float volume;
    bool loop;
    glm::quat orientation;
    bool stereo;
    bool ambisonic;
    bool ignorePenumbra;
    bool localOnly;
    float secondOffset;
};

Q_DECLARE_METATYPE(AudioInjectorOptions);

QScriptValue injectorOptionsToScriptValue(QScriptEngine* engine, const AudioInjectorOptions& injectorOptions);
void injectorOptionsFromScriptValue(const QScriptValue& object, AudioInjectorOptions& injectorOptions);

#endif // hifi_AudioInjectorOptions_h
