//
//  ScriptValue.cpp
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 9/2/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptValue.h"

#include "ScriptEngineLogging.h"


class ScriptValueProxyNull : public ScriptValueProxy {
public:
    virtual void release();
    virtual ScriptValueProxy* copy() const;

public:
    virtual ScriptValue call(const ScriptValue& thisObject = ScriptValue(), const ScriptValueList& args = ScriptValueList());
    virtual ScriptValue call(const ScriptValue& thisObject, const ScriptValue& arguments);
    virtual ScriptValue construct(const ScriptValueList& args = ScriptValueList());
    virtual ScriptValue construct(const ScriptValue& arguments);
    virtual ScriptValue data() const;
    virtual ScriptEnginePointer engine() const;
    virtual bool equals(const ScriptValue& other) const;
    virtual bool isArray() const;
    virtual bool isBool() const;
    virtual bool isError() const;
    virtual bool isFunction() const;
    virtual bool isNumber() const;
    virtual bool isNull() const;
    virtual bool isObject() const;
    virtual bool isString() const;
    virtual bool isUndefined() const;
    virtual bool isValid() const;
    virtual bool isVariant() const;
    virtual ScriptValueIteratorPointer newIterator() const;
    virtual ScriptValue property(const QString& name,
                                 const ScriptValue::ResolveFlags& mode = ScriptValue::ResolvePrototype) const;
    virtual ScriptValue property(quint32 arrayIndex,
                                 const ScriptValue::ResolveFlags& mode = ScriptValue::ResolvePrototype) const;
    virtual void setData(const ScriptValue& val);
    virtual void setProperty(const QString& name,
                             const ScriptValue& value,
                             const ScriptValue::PropertyFlags& flags = ScriptValue::KeepExistingFlags);
    virtual void setProperty(quint32 arrayIndex,
                             const ScriptValue& value,
                             const ScriptValue::PropertyFlags& flags = ScriptValue::KeepExistingFlags);
    virtual void setPrototype(const ScriptValue& prototype);
    virtual bool strictlyEquals(const ScriptValue& other) const;

    virtual bool toBool() const;
    virtual qint32 toInt32() const;
    virtual double toInteger() const;
    virtual double toNumber() const;
    virtual QString toString() const;
    virtual quint16 toUInt16() const;
    virtual quint32 toUInt32() const;
    virtual QVariant toVariant() const;
    virtual QObject* toQObject() const;
};

static ScriptValueProxyNull SCRIPT_VALUE_NULL;

ScriptValue::ScriptValue() : _proxy(&SCRIPT_VALUE_NULL) {}

void ScriptValueProxyNull::release() {
    // do nothing, we're a singlet
}

ScriptValueProxy* ScriptValueProxyNull::copy() const {
    // return ourselves, we're a singlet
    return const_cast <ScriptValueProxyNull*>(this);
}

ScriptValue ScriptValueProxyNull::call(const ScriptValue& thisObject, const ScriptValueList& args) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::call made to empty value");
    return ScriptValue();
}

ScriptValue ScriptValueProxyNull::call(const ScriptValue& thisObject, const ScriptValue& arguments) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::call made to empty value");
    return ScriptValue();
}

ScriptValue ScriptValueProxyNull::construct(const ScriptValueList& args) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::construct called on empty value");
    return ScriptValue();
}

ScriptValue ScriptValueProxyNull::construct(const ScriptValue& arguments) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::construct called on empty value");
    return ScriptValue();
}

ScriptValue ScriptValueProxyNull::data() const {
    return ScriptValue();
}

ScriptEnginePointer ScriptValueProxyNull::engine() const {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::engine called on empty value");
    return ScriptEnginePointer();
}

bool ScriptValueProxyNull::equals(const ScriptValue& other) const {
    return other.isUndefined();
}

bool ScriptValueProxyNull::isArray() const {
    return false;
}

bool ScriptValueProxyNull::isBool() const {
    return false;
}

bool ScriptValueProxyNull::isError() const {
    return false;
}

bool ScriptValueProxyNull::isFunction() const {
    return false;
}

bool ScriptValueProxyNull::isNumber() const {
    return false;
}

bool ScriptValueProxyNull::isNull() const {
    return false;
}

bool ScriptValueProxyNull::isObject() const {
    return false;
}

bool ScriptValueProxyNull::isString() const {
    return false;
}

bool ScriptValueProxyNull::isUndefined() const {
    return true;
}

bool ScriptValueProxyNull::isValid() const {
    return false;
}

bool ScriptValueProxyNull::isVariant() const {
    return false;
}

ScriptValueIteratorPointer ScriptValueProxyNull::newIterator() const {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::newIterator called on empty value");
    return ScriptValueIteratorPointer();
}

ScriptValue ScriptValueProxyNull::property(const QString& name, const ScriptValue::ResolveFlags& mode) const {
    return ScriptValue();
}

ScriptValue ScriptValueProxyNull::property(quint32 arrayIndex, const ScriptValue::ResolveFlags& mode) const {
    return ScriptValue();
}

void ScriptValueProxyNull::setData(const ScriptValue& val) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::setData called on empty value");
}

void ScriptValueProxyNull::setProperty(const QString& name,
                         const ScriptValue& value, const ScriptValue::PropertyFlags& flags) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::setProperty called on empty value");
}

void ScriptValueProxyNull::setProperty(quint32 arrayIndex,
                         const ScriptValue& value, const ScriptValue::PropertyFlags& flags) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::setProperty called on empty value");
}

void ScriptValueProxyNull::setPrototype(const ScriptValue& prototype) {
    Q_ASSERT(false);
    qCWarning(scriptengine_script, "ScriptValue::setPrototype called on empty value");
}

bool ScriptValueProxyNull::strictlyEquals(const ScriptValue& other) const {
    return !other.isValid();
}

bool ScriptValueProxyNull::toBool() const {
    return false;
}

qint32 ScriptValueProxyNull::toInt32() const {
    return 0;
}

double ScriptValueProxyNull::toInteger() const {
    return 0;
}

double ScriptValueProxyNull::toNumber() const {
    return 0;
}

QString ScriptValueProxyNull::toString() const {
    return QString();
}

quint16 ScriptValueProxyNull::toUInt16() const {
    return 0;
}

quint32 ScriptValueProxyNull::toUInt32() const {
    return 0;
}

QVariant ScriptValueProxyNull::toVariant() const {
    return QVariant();
}

QObject* ScriptValueProxyNull::toQObject() const {
    return nullptr;
}
