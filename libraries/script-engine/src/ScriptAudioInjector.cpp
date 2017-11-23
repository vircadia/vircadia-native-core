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

#include "ScriptEngineLogging.h"
#include "ScriptAudioInjector.h"

QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in) {
    // The AudioScriptingInterface::playSound method can return null, so we need to account for that.
    if (!in) {
        return QScriptValue(QScriptValue::NullValue);
    }

    // when the script goes down we want to cleanup the injector
    QObject::connect(engine, &QScriptEngine::destroyed, in, &ScriptAudioInjector::stopInjectorImmediately,
                     Qt::DirectConnection);

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
    if (!_injector.isNull()) {
        // we've been asked to delete after finishing, trigger a queued deleteLater here
        _injector->triggerDeleteAfterFinish();
    }
}

void ScriptAudioInjector::stopInjectorImmediately() {
    qCDebug(scriptengine) << "ScriptAudioInjector::stopInjectorImmediately called to stop audio injector immediately.";
    _injector->stop();
}
