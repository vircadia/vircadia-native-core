//
//  ScriptValueQtWrapper.cpp
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 5/16/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptValueQtWrapper.h"

#include "ScriptValueIteratorQtWrapper.h"

void ScriptValueQtWrapper::release() {
    delete this;
}

ScriptValueProxy* ScriptValueQtWrapper::copy() const {
    return new ScriptValueQtWrapper(_engine, _value);
}

ScriptValueQtWrapper* ScriptValueQtWrapper::unwrap(const ScriptValue& val) {
    return dynamic_cast<ScriptValueQtWrapper*>(val.ptr());
}

QScriptValue ScriptValueQtWrapper::fullUnwrap(const ScriptValue& value) const {
    ScriptValueQtWrapper* unwrapped = unwrap(value);
    if (unwrapped) {
        return unwrapped->toQtValue();
    }
    QVariant varValue = value.toVariant();
    return static_cast<QScriptEngine*>(_engine)->newVariant(varValue);
}

QScriptValue ScriptValueQtWrapper::fullUnwrap(ScriptEngineQtScript* engine, const ScriptValue& value) {
    ScriptValueQtWrapper* unwrapped = unwrap(value);
    if (unwrapped) {
        return unwrapped->toQtValue();
    }
    QVariant varValue = value.toVariant();
    return static_cast<QScriptEngine*>(engine)->newVariant(varValue);
}

ScriptValue ScriptValueQtWrapper::call(const ScriptValue& thisObject, const ScriptValueList& args) {
    QScriptValue qThis = fullUnwrap(thisObject);
    QScriptValueList qArgs;
    for (ScriptValueList::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
        qArgs.push_back(fullUnwrap(*iter));
    }
    QScriptValue result = _value.call(qThis, qArgs);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptValueQtWrapper::call(const ScriptValue& thisObject, const ScriptValue& arguments) {
    QScriptValue qThis = fullUnwrap(thisObject);
    QScriptValue qArgs = fullUnwrap(arguments);
    QScriptValue result = _value.call(qThis, qArgs);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptValueQtWrapper::construct(const ScriptValueList& args) {
    QScriptValueList qArgs;
    for (ScriptValueList::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
        qArgs.push_back(fullUnwrap(*iter));
    }
    QScriptValue result = _value.construct(qArgs);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptValueQtWrapper::construct(const ScriptValue& arguments) {
    QScriptValue unwrapped = fullUnwrap(arguments);
    QScriptValue result = _value.construct(unwrapped);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptValueQtWrapper::data() const {
    QScriptValue result = _value.data();
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptEnginePointer ScriptValueQtWrapper::engine() const {
    if (!_engine) {
        return ScriptEnginePointer();
    }
    return _engine->sharedFromThis();
}

ScriptValueIteratorPointer ScriptValueQtWrapper::newIterator() const {
    return ScriptValueIteratorPointer(new ScriptValueIteratorQtWrapper(_engine, _value));
}

ScriptValue ScriptValueQtWrapper::property(const QString& name, const ScriptValue::ResolveFlags& mode) const {
    QScriptValue result = _value.property(name, (QScriptValue::ResolveFlags)(int)mode);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValue ScriptValueQtWrapper::property(quint32 arrayIndex, const ScriptValue::ResolveFlags& mode) const {
    QScriptValue result = _value.property(arrayIndex, (QScriptValue::ResolveFlags)(int)mode);
    return ScriptValue(new ScriptValueQtWrapper(_engine, std::move(result)));
}

void ScriptValueQtWrapper::setData(const ScriptValue& value) {
    QScriptValue unwrapped = fullUnwrap(value);
    _value.setData(unwrapped);
}

void ScriptValueQtWrapper::setProperty(const QString& name, const ScriptValue& value, const ScriptValue::PropertyFlags& flags) {
    QScriptValue unwrapped = fullUnwrap(value);
    _value.setProperty(name, unwrapped, (QScriptValue::PropertyFlags)(int)flags);
}

void ScriptValueQtWrapper::setProperty(quint32 arrayIndex, const ScriptValue& value, const ScriptValue::PropertyFlags& flags) {
    QScriptValue unwrapped = fullUnwrap(value);
    _value.setProperty(arrayIndex, unwrapped, (QScriptValue::PropertyFlags)(int)flags);
}

void ScriptValueQtWrapper::setPrototype(const ScriptValue& prototype) {
    ScriptValueQtWrapper* unwrappedPrototype = unwrap(prototype);
    if (unwrappedPrototype) {
        _value.setPrototype(unwrappedPrototype->toQtValue());
    }
}

bool ScriptValueQtWrapper::strictlyEquals(const ScriptValue& other) const {
    ScriptValueQtWrapper* unwrappedOther = unwrap(other);
    return unwrappedOther ? _value.strictlyEquals(unwrappedOther->toQtValue()) : false;
}

bool ScriptValueQtWrapper::toBool() const {
    return _value.toBool();
}

qint32 ScriptValueQtWrapper::toInt32() const {
    return _value.toInt32();
}

double ScriptValueQtWrapper::toInteger() const {
    return _value.toInteger();
}

double ScriptValueQtWrapper::toNumber() const {
    return _value.toNumber();
}

QString ScriptValueQtWrapper::toString() const {
    return _value.toString();
}

quint16 ScriptValueQtWrapper::toUInt16() const {
    return _value.toUInt16();
}

quint32 ScriptValueQtWrapper::toUInt32() const {
    return _value.toUInt32();
}

QVariant ScriptValueQtWrapper::toVariant() const {
    return _value.toVariant();
}

QObject* ScriptValueQtWrapper::toQObject() const {
    return _value.toQObject();
}

bool ScriptValueQtWrapper::equals(const ScriptValue& other) const {
    ScriptValueQtWrapper* unwrappedOther = unwrap(other);
    return unwrappedOther ? _value.equals(unwrappedOther->toQtValue()) : false;
}

bool ScriptValueQtWrapper::isArray() const {
    return _value.isArray();
}

bool ScriptValueQtWrapper::isBool() const {
    return _value.isBool();
}

bool ScriptValueQtWrapper::isError() const {
    return _value.isError();
}

bool ScriptValueQtWrapper::isFunction() const {
    return _value.isFunction();
}

bool ScriptValueQtWrapper::isNumber() const {
    return _value.isNumber();
}

bool ScriptValueQtWrapper::isNull() const {
    return _value.isNull();
}

bool ScriptValueQtWrapper::isObject() const {
    return _value.isObject();
}

bool ScriptValueQtWrapper::isString() const {
    return _value.isString();
}

bool ScriptValueQtWrapper::isUndefined() const {
    return _value.isUndefined();
}

bool ScriptValueQtWrapper::isValid() const {
    return _value.isValid();
}

bool ScriptValueQtWrapper::isVariant() const {
    return _value.isVariant();
}
