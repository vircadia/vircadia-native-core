//
//  ScriptInitializerMixin.h
//  libraries/shared/src/shared
//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.

#pragma once

#include <functional>
#include <mutex>
#include "../DependencyManager.h"

class QScriptEngine;
class ScriptEngine;

template <typename T> class ScriptInitializerMixin {
public:
    using ScriptInitializer = std::function<void(T)>;
    virtual void registerScriptInitializer(ScriptInitializer initializer) {
        InitializerLock lock(_scriptInitializerMutex);
        _scriptInitializers.push_back(initializer);
    }
    virtual int runScriptInitializers(T engine) {
        InitializerLock lock(_scriptInitializerMutex);
        return std::count_if(_scriptInitializers.begin(), _scriptInitializers.end(),
            [engine](auto initializer){ initializer(engine); return true; }
        );
    }
    virtual ~ScriptInitializerMixin() {}
protected:
    std::mutex _scriptInitializerMutex;
    using InitializerLock = std::lock_guard<std::mutex>;
    std::list<ScriptInitializer> _scriptInitializers;
};

class ScriptInitializers : public ScriptInitializerMixin<QScriptEngine*>, public Dependency {
public:
    // Lightweight `QScriptEngine*` initializer (only depends on built-in Qt components)
    // example registration:
    // eg: [&](QScriptEngine* engine) {
    //     engine->globalObject().setProperties("API", engine->newQObject(...instance...))
    // };
};
