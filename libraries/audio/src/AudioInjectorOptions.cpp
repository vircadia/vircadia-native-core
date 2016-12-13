//
//  AudioInjectorOptions.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <RegisteredMetaTypes.h>

#include "AudioInjectorOptions.h"

AudioInjectorOptions::AudioInjectorOptions() :
    position(0.0f, 0.0f, 0.0f),
    volume(1.0f),
    loop(false),
    orientation(glm::vec3(0.0f, 0.0f, 0.0f)),
    stereo(false),
    ambisonic(false),
    ignorePenumbra(false),
    localOnly(false),
    secondOffset(0.0)
{

}

QScriptValue injectorOptionsToScriptValue(QScriptEngine* engine, const AudioInjectorOptions& injectorOptions) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("position", vec3toScriptValue(engine, injectorOptions.position));
    obj.setProperty("volume", injectorOptions.volume);
    obj.setProperty("loop", injectorOptions.loop);
    obj.setProperty("orientation", quatToScriptValue(engine, injectorOptions.orientation));
    obj.setProperty("ignorePenumbra", injectorOptions.ignorePenumbra);
    obj.setProperty("localOnly", injectorOptions.localOnly);
    obj.setProperty("secondOffset", injectorOptions.secondOffset);
    return obj;
}

void injectorOptionsFromScriptValue(const QScriptValue& object, AudioInjectorOptions& injectorOptions) {
    if (object.property("position").isValid()) {
        vec3FromScriptValue(object.property("position"), injectorOptions.position);
    }
    
    if (object.property("volume").isValid()) {
        injectorOptions.volume = object.property("volume").toNumber();
    }
    
    if (object.property("loop").isValid()) {
        injectorOptions.loop = object.property("loop").toBool();
    }
    
    if (object.property("orientation").isValid()) {
        quatFromScriptValue(object.property("orientation"), injectorOptions.orientation);
    }
    
    if (object.property("ignorePenumbra").isValid()) {
        injectorOptions.ignorePenumbra = object.property("ignorePenumbra").toBool();
    }
    
    if (object.property("localOnly").isValid()) {
        injectorOptions.localOnly = object.property("localOnly").toBool();
    }
    
    if (object.property("secondOffset").isValid()) {
        injectorOptions.secondOffset = object.property("secondOffset").toNumber();
    }
 }