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

#include "ScriptEngineLogging.h"

QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in) {
    // The AudioScriptingInterface::playSound method can return null, so we need to account for that.
    if (!in) {
        return QScriptValue(QScriptValue::NullValue);
    }

    // when the script goes down we want to cleanup the injector
    QObject::connect(engine, &QScriptEngine::destroyed, DependencyManager::get<AudioInjectorManager>().data(), [&] {
        qCDebug(scriptengine) << "Script was shutdown, stopping an injector";
        // FIXME: this doesn't work and leaves the injectors lying around
        //DependencyManager::get<AudioInjectorManager>()->stop(in->_injector);
    });

    return engine->newQObject(in, QScriptEngine::ScriptOwnership);
}

void injectorFromScriptValue(const QScriptValue& object, ScriptAudioInjector*& out) {
    out = qobject_cast<ScriptAudioInjector*>(object.toQObject());
}

ScriptAudioInjector::ScriptAudioInjector(const AudioInjectorPointer& injector) :
    _injector(injector)
{
    QObject::connect(injector.data(), &AudioInjector::finished, this, &ScriptAudioInjector::finished);
}

ScriptAudioInjector::~ScriptAudioInjector() {
    DependencyManager::get<AudioInjectorManager>()->stop(_injector);
}