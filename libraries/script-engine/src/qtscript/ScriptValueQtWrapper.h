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
    virtual void release();
    virtual ScriptValueProxy* copy() const;

public:  // ScriptValue implementation
    virtual ScriptValue call(const ScriptValue& thisObject = ScriptValue(), const ScriptValueList& args = ScriptValueList());
    virtual ScriptValue call(const ScriptValue& thisObject, const ScriptValue& arguments);
    virtual ScriptValue construct(const ScriptValueList& args = ScriptValueList());
    virtual ScriptValue construct(const ScriptValue& arguments);
    virtual ScriptValue data() const;
    virtual ScriptEnginePointer engine() const;
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
    virtual bool toBool() const;
    virtual qint32 toInt32() const;
    virtual double toInteger() const;
    virtual double toNumber() const;
    virtual QString toString() const;
    virtual quint16 toUInt16() const;
    virtual quint32 toUInt32() const;
    virtual QVariant toVariant() const;
    virtual QObject* toQObject() const;

private: // helper functions
    QScriptValue fullUnwrap(const ScriptValue& value) const;

private: // storage
    QPointer<ScriptEngineQtScript> _engine;
    QScriptValue _value;
};

#endif  // hifi_ScriptValueQtWrapper_h

/// @}
