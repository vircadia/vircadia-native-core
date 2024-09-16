//
//  ScriptContextQtWrapper.cpp
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 5/22/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptContextQtWrapper.h"

#include <QtScript/QScriptContext>

#include "ScriptEngineQtScript.h"
#include "ScriptValueQtWrapper.h"

ScriptContextQtWrapper* ScriptContextQtWrapper::unwrap(ScriptContext* val) {
    if (!val) {
        return nullptr;
    }

    return dynamic_cast<ScriptContextQtWrapper*>(val);
}

int ScriptContextQtWrapper::argumentCount() const {
    return _context->argumentCount();
}

ScriptValue ScriptContextQtWrapper::argument(int index) const {
    QScriptValue result = _context->argument(index);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

QStringList ScriptContextQtWrapper::backtrace() const {
    return _context->backtrace();
}

ScriptValue ScriptContextQtWrapper::callee() const {
    QScriptValue result = _context->callee();
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptEnginePointer ScriptContextQtWrapper::engine() const {
    return _engine->shared_from_this();
}

ScriptFunctionContextPointer ScriptContextQtWrapper::functionContext() const {
    return std::make_shared<ScriptFunctionContextQtWrapper>(_context);
}

ScriptContextPointer ScriptContextQtWrapper::parentContext() const {
    QScriptContext* result = _context->parentContext();
    return result ? std::make_shared<ScriptContextQtWrapper>(_engine, result) : ScriptContextPointer();
}

ScriptValue ScriptContextQtWrapper::thisObject() const {
    QScriptValue result = _context->thisObject();
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptContextQtWrapper::throwError(const QString& text) {
    QScriptValue result = _context->throwError(text);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptContextQtWrapper::throwValue(const ScriptValue& value) {
    ScriptValueQtWrapper* unwrapped = ScriptValueQtWrapper::unwrap(value);
    if (!unwrapped) {
        return _engine->undefinedValue();
    }
    QScriptValue result = _context->throwValue(unwrapped->toQtValue());
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}


QString ScriptFunctionContextQtWrapper::fileName() const {
    return _value.fileName();
}

QString ScriptFunctionContextQtWrapper::functionName() const {
    return _value.functionName();
}

ScriptFunctionContext::FunctionType ScriptFunctionContextQtWrapper::functionType() const {
    return static_cast<ScriptFunctionContext::FunctionType>(_value.functionType());
}

int ScriptFunctionContextQtWrapper::lineNumber() const {
    return _value.lineNumber();
}
