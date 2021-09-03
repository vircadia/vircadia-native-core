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
    virtual void release() override;
    virtual ScriptValueProxy* copy() const override;

public:
    virtual ScriptValue call(const ScriptValue& thisObject = ScriptValue(), const ScriptValueList& args = ScriptValueList()) override;
    virtual ScriptValue call(const ScriptValue& thisObject, const ScriptValue& arguments) override;
    virtual ScriptValue construct(const ScriptValueList& args = ScriptValueList()) override;
    virtual ScriptValue construct(const ScriptValue& arguments) override;
    virtual ScriptValue data() const override;
    virtual ScriptEnginePointer engine() const override;
    virtual bool equals(const ScriptValue& other) const override;
    virtual bool isArray() const override;
    virtual bool isBool() const override;
    virtual bool isError() const override;
    virtual bool isFunction() const override;
    virtual bool isNumber() const override;
    virtual bool isNull() const override;
    virtual bool isObject() const override;
    virtual bool isString() const override;
    virtual bool isUndefined() const override;
    virtual bool isValid() const override;
    virtual bool isVariant() const override;
    virtual ScriptValueIteratorPointer newIterator() const override;
    virtual ScriptValue property(const QString& name,
                                 const ScriptValue::ResolveFlags& mode = ScriptValue::ResolvePrototype) const override;
    virtual ScriptValue property(quint32 arrayIndex,
                                 const ScriptValue::ResolveFlags& mode = ScriptValue::ResolvePrototype) const override;
    virtual void setData(const ScriptValue& val);
    virtual void setProperty(const QString& name,
                             const ScriptValue& value,
                             const ScriptValue::PropertyFlags& flags = ScriptValue::KeepExistingFlags) override;
    virtual void setProperty(quint32 arrayIndex,
                             const ScriptValue& value,
                             const ScriptValue::PropertyFlags& flags = ScriptValue::KeepExistingFlags) override;
    virtual void setPrototype(const ScriptValue& prototype) override;
    virtual bool strictlyEquals(const ScriptValue& other) const override;

    virtual bool toBool() const override;
    virtual qint32 toInt32() const override;
    virtual double toInteger() const override;
    virtual double toNumber() const override;
    virtual QString toString() const override;
    virtual quint16 toUInt16() const override;
    virtual quint32 toUInt32() const override;
    virtual QVariant toVariant() const override;
    virtual QObject* toQObject() const override;
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
