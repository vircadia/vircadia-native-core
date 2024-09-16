//
//  Scriptable.h
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 5/1/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_Scriptable_h
#define hifi_Scriptable_h

#include <memory>

#include "ScriptContext.h"
#include "ScriptValue.h"

class ScriptEngine;
using ScriptEnginePointer = std::shared_ptr<ScriptEngine>;

/// [ScriptInterface] Provides an engine-independent interface for QScriptable
class Scriptable {
public:
    static inline ScriptEnginePointer engine();
    static ScriptContext* context();
    static inline ScriptValue thisObject();
    static inline int argumentCount();
    static inline ScriptValue argument(int index);

    static void setContext(ScriptContext* context);
};

ScriptEnginePointer Scriptable::engine() {
    ScriptContext* scriptContext = context();
    return scriptContext ? scriptContext->engine() : nullptr;
}

ScriptValue Scriptable::thisObject() {
    ScriptContext* scriptContext = context();
    return scriptContext ? scriptContext->thisObject() : ScriptValue();
}

int Scriptable::argumentCount() {
    ScriptContext* scriptContext = context();
    return scriptContext ? scriptContext->argumentCount() : 0;
}

ScriptValue Scriptable::argument(int index) {
    ScriptContext* scriptContext = context();
    return scriptContext ? scriptContext->argument(index) : ScriptValue();
}

#endif // hifi_Scriptable_h

/// @}
