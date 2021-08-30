//
//  ScriptValueQtWrapper.cpp
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 5/16/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptValueQtWrapper.h"

#include "ScriptValueIteratorQtWrapper.h"

ScriptValueQtWrapper* ScriptValueQtWrapper::unwrap(ScriptValuePointer val) {
    if (!val) {
        return nullptr;
    }

    return dynamic_cast<ScriptValueQtWrapper*>(val.data());
}

QScriptValue ScriptValueQtWrapper::fullUnwrap(const ScriptValuePointer& value) const {
    if (!value) {
        return QScriptValue();
    }
    ScriptValueQtWrapper* unwrapped = unwrap(value);
    if (unwrapped) {
        return unwrapped->toQtValue();
    }
    QVariant varValue = value->toVariant();
    return static_cast<QScriptEngine*>(_engine)->newVariant(varValue);
}

QScriptValue ScriptValueQtWrapper::fullUnwrap(ScriptEngineQtScript* engine, const ScriptValuePointer& value) {
    if (!value) {
        return QScriptValue();
    }
    ScriptValueQtWrapper* unwrapped = unwrap(value);
    if (unwrapped) {
        return unwrapped->toQtValue();
    }
    QVariant varValue = value->toVariant();
    return static_cast<QScriptEngine*>(engine)->newVariant(varValue);
}

ScriptValuePointer ScriptValueQtWrapper::call(const ScriptValuePointer& thisObject, const ScriptValueList& args) {
    QScriptValue qThis = fullUnwrap(thisObject);
    QScriptValueList qArgs;
    for (ScriptValueList::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
        qArgs.push_back(fullUnwrap(*iter));
    }
    QScriptValue result = _value.call(qThis, qArgs);
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValuePointer ScriptValueQtWrapper::call(const ScriptValuePointer& thisObject, const ScriptValuePointer& arguments) {
    QScriptValue qThis = fullUnwrap(thisObject);
    QScriptValue qArgs = fullUnwrap(arguments);
    QScriptValue result = _value.call(qThis, qArgs);
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValuePointer ScriptValueQtWrapper::construct(const ScriptValueList& args) {
    QScriptValueList qArgs;
    for (ScriptValueList::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
        qArgs.push_back(fullUnwrap(*iter));
    }
    QScriptValue result = _value.construct(qArgs);
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValuePointer ScriptValueQtWrapper::construct(const ScriptValuePointer& arguments) {
    QScriptValue unwrapped = fullUnwrap(arguments);
    QScriptValue result = _value.construct(unwrapped);
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValuePointer ScriptValueQtWrapper::data() const {
    QScriptValue result = _value.data();
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptEnginePointer ScriptValueQtWrapper::engine() const {
    if (!_engine) {
        return ScriptEnginePointer();
    }
    return _engine->sharedFromThis();
}

ScriptValueIteratorPointer ScriptValueQtWrapper::newIterator() {
    return ScriptValueIteratorPointer(new ScriptValueIteratorQtWrapper(_engine, _value));
}

ScriptValuePointer ScriptValueQtWrapper::property(const QString& name, const ResolveFlags& mode) const {
    QScriptValue result = _value.property(name, (QScriptValue::ResolveFlags)(int)mode);
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

ScriptValuePointer ScriptValueQtWrapper::property(quint32 arrayIndex, const ResolveFlags& mode) const {
    QScriptValue result = _value.property(arrayIndex, (QScriptValue::ResolveFlags)(int)mode);
    return ScriptValuePointer(new ScriptValueQtWrapper(_engine, std::move(result)));
}

void ScriptValueQtWrapper::setData(const ScriptValuePointer& value) {
    QScriptValue unwrapped = fullUnwrap(value);
    _value.setData(unwrapped);
}

void ScriptValueQtWrapper::setProperty(const QString& name, const ScriptValuePointer& value, const PropertyFlags& flags) {
    QScriptValue unwrapped = fullUnwrap(value);
    _value.setProperty(name, unwrapped, (QScriptValue::PropertyFlags)(int)flags);
}

void ScriptValueQtWrapper::setProperty(quint32 arrayIndex, const ScriptValuePointer& value, const PropertyFlags& flags) {
    QScriptValue unwrapped = fullUnwrap(value);
    _value.setProperty(arrayIndex, unwrapped, (QScriptValue::PropertyFlags)(int)flags);
}

void ScriptValueQtWrapper::setPrototype(const ScriptValuePointer& prototype) {
    ScriptValueQtWrapper* unwrappedPrototype = unwrap(prototype);
    if (unwrappedPrototype) {
        _value.setPrototype(unwrappedPrototype->toQtValue());
    }
}

bool ScriptValueQtWrapper::strictlyEquals(const ScriptValuePointer& other) const {
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

bool ScriptValueQtWrapper::equalsInternal(const ScriptValuePointer& other) const {
    ScriptValueQtWrapper* unwrappedOther = unwrap(other);
    return unwrappedOther ? _value.equals(unwrappedOther->toQtValue()) : false;
}

bool ScriptValueQtWrapper::isArrayInternal() const {
    return _value.isArray();
}

bool ScriptValueQtWrapper::isBoolInternal() const {
    return _value.isBool();
}

bool ScriptValueQtWrapper::isErrorInternal() const {
    return _value.isError();
}

bool ScriptValueQtWrapper::isFunctionInternal() const {
    return _value.isFunction();
}

bool ScriptValueQtWrapper::isNumberInternal() const {
    return _value.isNumber();
}

bool ScriptValueQtWrapper::isNullInternal() const {
    return _value.isNull();
}

bool ScriptValueQtWrapper::isObjectInternal() const {
    return _value.isObject();
}

bool ScriptValueQtWrapper::isStringInternal() const {
    return _value.isString();
}

bool ScriptValueQtWrapper::isUndefinedInternal() const {
    return _value.isUndefined();
}

bool ScriptValueQtWrapper::isValidInternal() const {
    return _value.isValid();
}

bool ScriptValueQtWrapper::isVariantInternal() const {
    return _value.isVariant();
}
