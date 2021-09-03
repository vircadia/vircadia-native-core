//
//  ScriptValueQtWrapper.h
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 5/16/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptValueQtWrapper_h
#define hifi_ScriptValueQtWrapper_h

#include <QtCore/QPointer>
#include <QtScript/QScriptValue>

#include <utility>

#include "../ScriptValue.h"
#include "ScriptEngineQtScript.h"

/// [QtScript] Implements ScriptValue for QtScript and translates calls for QScriptValue
class ScriptValueQtWrapper : public ScriptValueProxy {
public: // construction
    inline ScriptValueQtWrapper(ScriptEngineQtScript* engine, const QScriptValue& value) :
        _engine(engine), _value(value) {}
    inline ScriptValueQtWrapper(ScriptEngineQtScript* engine, QScriptValue&& value) :
        _engine(engine), _value(std::move(value)) {}
    static ScriptValueQtWrapper* unwrap(const ScriptValue& val);
    inline const QScriptValue& toQtValue() const { return _value; }
    static QScriptValue fullUnwrap(ScriptEngineQtScript* engine, const ScriptValue& value);

public:
    virtual void release() override;
    virtual ScriptValueProxy* copy() const override;

public:  // ScriptValue implementation
    virtual ScriptValue call(const ScriptValue& thisObject = ScriptValue(),
                             const ScriptValueList& args = ScriptValueList()) override;
    virtual ScriptValue call(const ScriptValue& thisObject, const ScriptValue& arguments) override;
    virtual ScriptValue construct(const ScriptValueList& args = ScriptValueList()) override;
    virtual ScriptValue construct(const ScriptValue& arguments) override;
    virtual ScriptValue data() const override;
    virtual ScriptEnginePointer engine() const override;
    virtual ScriptValueIteratorPointer newIterator() const override;
    virtual ScriptValue property(const QString& name,
                                 const ScriptValue::ResolveFlags& mode = ScriptValue::ResolvePrototype) const override;
    virtual ScriptValue property(quint32 arrayIndex,
                                 const ScriptValue::ResolveFlags& mode = ScriptValue::ResolvePrototype) const override;
    virtual void setData(const ScriptValue& val) override;
    virtual void setProperty(const QString& name,
                             const ScriptValue& value,
                             const ScriptValue::PropertyFlags& flags = ScriptValue::KeepExistingFlags) override;
    virtual void setProperty(quint32 arrayIndex,
                             const ScriptValue& value,
                             const ScriptValue::PropertyFlags& flags = ScriptValue::KeepExistingFlags) override;
    virtual void setPrototype(const ScriptValue& prototype) override;
    virtual bool strictlyEquals(const ScriptValue& other) const override;

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
    virtual bool toBool() const override;
    virtual qint32 toInt32() const override;
    virtual double toInteger() const override;
    virtual double toNumber() const override;
    virtual QString toString() const override;
    virtual quint16 toUInt16() const override;
    virtual quint32 toUInt32() const override;
    virtual QVariant toVariant() const override;
    virtual QObject* toQObject() const override;

private: // helper functions
    QScriptValue fullUnwrap(const ScriptValue& value) const;

private: // storage
    QPointer<ScriptEngineQtScript> _engine;
    QScriptValue _value;
};

#endif  // hifi_ScriptValueQtWrapper_h

/// @}
