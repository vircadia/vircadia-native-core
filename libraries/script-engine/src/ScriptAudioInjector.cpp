//
//  ScriptAudioInjector.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2015-02-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptAudioInjector.h"

QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in) {
    // when the script goes down we want to cleanup the injector
    QObject::connect(engine, &QScriptEngine::destroyed, in, &ScriptAudioInjector::stopInjectorImmediately);
    
    return engine->newQObject(in, QScriptEngine::ScriptOwnership);
}

void injectorFromScriptValue(const QScriptValue& object, ScriptAudioInjector*& out) {
    out = qobject_cast<ScriptAudioInjector*>(object.toQObject());
}

ScriptAudioInjector::ScriptAudioInjector(AudioInjector* injector) :
    _injector(injector)
{
    
}

ScriptAudioInjector::~ScriptAudioInjector() {
}

void ScriptAudioInjector::stopInjectorImmediately() {
    _injector->stopAndDeleteLater();
}
