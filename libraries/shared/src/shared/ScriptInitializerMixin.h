//
//  ScriptInitializerMixin.h
//  libraries/shared/src/shared
//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.

#pragma once

#include <functional>
#include <QSharedPointer>
#include "../DependencyManager.h"

class QScriptEngine;
class ScriptEngine;

class ScriptInitializerMixin : public QObject, public Dependency {
    Q_OBJECT
public:
    // Lightweight `QScriptEngine*` initializer (only depends on built-in Qt components)
    // example registration:
    // eg: [&](QScriptEngine* engine) -> bool {
    //     engine->globalObject().setProperties("API", engine->newQObject(...instance...))
    //     return true;
    // }
    using NativeScriptInitializer = std::function<void(QScriptEngine*)>;
    virtual bool registerNativeScriptInitializer(NativeScriptInitializer initializer) = 0;

    // Heavyweight `ScriptEngine*` initializer (tightly coupled to Interface and script-engine library internals)
    // eg: [&](ScriptEnginePointer scriptEngine) -> bool {
    //     engine->registerGlobalObject("API", ...instance..);
    //     return true;
    // }
    using ScriptEnginePointer = QSharedPointer<ScriptEngine>;
    using ScriptInitializer = std::function<void(ScriptEnginePointer)>;
    virtual bool registerScriptInitializer(ScriptInitializer initializer) { return false; };
};
