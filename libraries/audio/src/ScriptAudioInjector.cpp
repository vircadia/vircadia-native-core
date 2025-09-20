//
//  ScriptAudioInjector.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2015-02-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptAudioInjector.h"

#include <ScriptEngine.h>
#include <ScriptEngineCast.h>
#include <ScriptEngineLogging.h>
#include <ScriptManager.h>
#include <ScriptValue.h>

STATIC_SCRIPT_INITIALIZER(+[](ScriptManager* manager) {
    auto scriptEngine = manager->engine().get();

    scriptRegisterMetaType(scriptEngine, injectorToScriptValue, injectorFromScriptValue);
});

ScriptValue injectorToScriptValue(ScriptEngine* engine, ScriptAudioInjector* const& in) {
    // The AudioScriptingInterface::playSound method can return null, so we need to account for that.
    if (!in) {
        return engine->nullValue();
    }

    return engine->newQObject(in, ScriptEngine::ScriptOwnership);
}

bool injectorFromScriptValue(const ScriptValue& object, ScriptAudioInjector*& out) {
    out = qobject_cast<ScriptAudioInjector*>(object.toQObject());
    return true;
}

ScriptAudioInjector::ScriptAudioInjector(const AudioInjectorPointer& injector) :
    _injector(injector)
{
    QObject::connect(injector.data(), &AudioInjector::finished, this, &ScriptAudioInjector::finished);
    connect(injector.data(), &QObject::destroyed, this, &QObject::deleteLater);
}

ScriptAudioInjector::~ScriptAudioInjector() {
    const auto audioInjectorManager = DependencyManager::get<AudioInjectorManager>();
    // AudioInjectorManager may have been destroyed on application shutdown.
    if (audioInjectorManager) {
        audioInjectorManager->stop(_injector);
    }
}
