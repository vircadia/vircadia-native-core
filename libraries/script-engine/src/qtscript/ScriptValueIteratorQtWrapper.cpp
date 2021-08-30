//
//  ScriptValueIteratorQtWrapper.cpp
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 8/29/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptValueIteratorQtWrapper.h"

ScriptValue::PropertyFlags ScriptValueIteratorQtWrapper::flags() const {
    return (ScriptValue::PropertyFlags)(int)_value.flags();
}

bool ScriptValueIteratorQtWrapper::hasNext() const {
    return _value.hasNext();
}

QString ScriptValueIteratorQtWrapper::name() const {
    return _value.name();
}

void ScriptValueIteratorQtWrapper::next() {
    _value.next();
}

ScriptValuePointer ScriptValueIteratorQtWrapper::value() const {
    QScriptValue result = _value.value();
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}
