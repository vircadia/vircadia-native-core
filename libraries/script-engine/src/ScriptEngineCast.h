//
//  ScriptEngineCast.h
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 5/9/2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptEngineCast_h
#define hifi_ScriptEngineCast_h

// Object conversion helpers (copied from QScriptEngine)

#include <QtCore/QMetaType>

#include "ScriptEngine.h"
#include "ScriptValue.h"

template <typename T>
inline ScriptValue scriptValueFromValue(ScriptEngine* engine, const T& t) {
    if (!engine) {
        return ScriptValue();
    }

    return engine->create(qMetaTypeId<T>(), &t);
}

template <>
inline ScriptValue scriptValueFromValue<QVariant>(ScriptEngine* engine, const QVariant& v) {
    if (!engine) {
        return ScriptValue();
    }

    return engine->create(v.userType(), v.data());
}

template <typename T>
inline T scriptvalue_cast(const ScriptValue& value) {
    T t;
    const int id = qMetaTypeId<T>();

    auto engine = value.engine();
    if (engine && engine->convert(value, id, &t)) {
        return t;
    } else if (value.isVariant()) {
        return qvariant_cast<T>(value.toVariant());
    }

    return T();
}

template <>
inline QVariant scriptvalue_cast<QVariant>(const ScriptValue& value) {
    return value.toVariant();
}

template <typename T>
int scriptRegisterMetaType(ScriptEngine* eng,
                           ScriptValue (*toScriptValue)(ScriptEngine*, const T& t),
                           void (*fromScriptValue)(const ScriptValue&, T& t),
                           const ScriptValue& prototype = ScriptValue(),
                           T* = 0)
{
    const int id = qRegisterMetaType<T>();  // make sure it's registered
    eng->registerCustomType(id, reinterpret_cast<ScriptEngine::MarshalFunction>(toScriptValue),
                            reinterpret_cast<ScriptEngine::DemarshalFunction>(fromScriptValue), prototype);
    return id;
}

template <class Container>
ScriptValue scriptValueFromSequence(ScriptEngine* eng, const Container& cont) {
    ScriptValue a = eng->newArray();
    typename Container::const_iterator begin = cont.begin();
    typename Container::const_iterator end = cont.end();
    typename Container::const_iterator it;
    quint32 i;
    for (it = begin, i = 0; it != end; ++it, ++i) {
        a.setProperty(i, eng->toScriptValue(*it));
    }
    return a;
}

template <class Container>
void scriptValueToSequence(const ScriptValue& value, Container& cont) {
    quint32 len = value.property(QLatin1String("length")).toUInt32();
    for (quint32 i = 0; i < len; ++i) {
        ScriptValue item = value.property(i);
        cont.push_back(scriptvalue_cast<typename Container::value_type>(item));
    }
}

template <typename T>
int scriptRegisterSequenceMetaType(ScriptEngine* engine,
                                   const ScriptValue& prototype = ScriptValue(),
                                   T* = 0) {
    return scriptRegisterMetaType<T>(engine, scriptValueFromSequence, scriptValueToSequence, prototype);
}

#endif // hifi_ScriptEngineCast_h

/// @}
