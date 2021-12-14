//
//  ScriptEngineQtScript_cast.cpp
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson 12/9/2021
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngineQtScript.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtScript/QScriptEngine>

#include "../ScriptValueIterator.h"

#include "ScriptObjectQtProxy.h"
#include "ScriptValueQtWrapper.h"

template <int i>
class CustomTypeInstance {
public:
    static ScriptEngine::MarshalFunction marshalFunc;
    static ScriptEngine::DemarshalFunction demarshalFunc;

    static QScriptValue internalMarshalFunc(QScriptEngine* engine, const void* src) {
        ScriptEngineQtScript* unwrappedEngine = static_cast<ScriptEngineQtScript*>(engine);
        ScriptValue dest = marshalFunc(unwrappedEngine, src);
        return ScriptValueQtWrapper::fullUnwrap(unwrappedEngine, dest);
    }

    static void internalDemarshalFunc(const QScriptValue& src, void* dest) {
        ScriptEngineQtScript* unwrappedEngine = static_cast<ScriptEngineQtScript*>(src.engine());
        ScriptValue wrappedSrc(new ScriptValueQtWrapper(unwrappedEngine, src));
        demarshalFunc(wrappedSrc, dest);
    }
};
template <int i>
ScriptEngine::MarshalFunction CustomTypeInstance<i>::marshalFunc;
template <int i>
ScriptEngine::DemarshalFunction CustomTypeInstance<i>::demarshalFunc;

// I would *LOVE* it if there was a different way to do this, jeez!
// Qt requires two functions that have no parameters that give any context,
// one of the must return a QScriptValue (so we can't void* them into generics and stick them in the templates).
// This *has* to be done via templates but the whole point of this is to avoid leaking types into the rest of
// the system that would require anyone other than us to have a dependency on QtScript
#define CUSTOM_TYPE_ENTRY(idx) \
        case idx: \
            CustomTypeInstance<idx>::marshalFunc = marshalFunc; \
            CustomTypeInstance<idx>::demarshalFunc = demarshalFunc; \
            internalMarshalFunc = CustomTypeInstance<idx>::internalMarshalFunc; \
            internalDemarshalFunc = CustomTypeInstance<idx>::internalDemarshalFunc; \
            break;
#define CUSTOM_TYPE_ENTRY_10(idx) \
        CUSTOM_TYPE_ENTRY((idx * 10)); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 1); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 2); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 3); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 4); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 5); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 6); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 7); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 8); \
        CUSTOM_TYPE_ENTRY((idx * 10) + 9);

void ScriptEngineQtScript::setDefaultPrototype(int metaTypeId, const ScriptValue& prototype) {
    ScriptValueQtWrapper* unwrappedPrototype = ScriptValueQtWrapper::unwrap(prototype);
    if (unwrappedPrototype) {
        const QScriptValue& scriptPrototype = unwrappedPrototype->toQtValue();
        QScriptEngine::setDefaultPrototype(metaTypeId, scriptPrototype);
 
        QMutexLocker guard(&_customTypeProtect);
        _customPrototypes.insert(metaTypeId, scriptPrototype);
    }
}

void ScriptEngineQtScript::registerCustomType(int type,
                                              ScriptEngine::MarshalFunction marshalFunc,
                                              ScriptEngine::DemarshalFunction demarshalFunc)
{
    QScriptEngine::MarshalFunction internalMarshalFunc;
    QScriptEngine::DemarshalFunction internalDemarshalFunc;

    {
        QMutexLocker guard(&_customTypeProtect);

        // storing it in a map for our own benefit
        CustomMarshal& customType = _customTypes.insert(type, CustomMarshal()).value();
        customType.demarshalFunc = demarshalFunc;
        customType.marshalFunc = marshalFunc;

        // creating a conversion for QtScript's benefit
        if (_nextCustomType >= 300) {  // have we ran out of translators?
            Q_ASSERT(false);
            return;
        }

        switch (_nextCustomType++) {
            CUSTOM_TYPE_ENTRY_10(0);
            CUSTOM_TYPE_ENTRY_10(1);
            CUSTOM_TYPE_ENTRY_10(2);
            CUSTOM_TYPE_ENTRY_10(3);
            CUSTOM_TYPE_ENTRY_10(4);
            CUSTOM_TYPE_ENTRY_10(5);
            CUSTOM_TYPE_ENTRY_10(6);
            CUSTOM_TYPE_ENTRY_10(7);
            CUSTOM_TYPE_ENTRY_10(8);
            CUSTOM_TYPE_ENTRY_10(9);
            CUSTOM_TYPE_ENTRY_10(10);
            CUSTOM_TYPE_ENTRY_10(11);
            CUSTOM_TYPE_ENTRY_10(12);
            CUSTOM_TYPE_ENTRY_10(13);
            CUSTOM_TYPE_ENTRY_10(14);
            CUSTOM_TYPE_ENTRY_10(15);
            CUSTOM_TYPE_ENTRY_10(16);
            CUSTOM_TYPE_ENTRY_10(17);
            CUSTOM_TYPE_ENTRY_10(18);
            CUSTOM_TYPE_ENTRY_10(19);
            CUSTOM_TYPE_ENTRY_10(20);
            CUSTOM_TYPE_ENTRY_10(21);
            CUSTOM_TYPE_ENTRY_10(22);
            CUSTOM_TYPE_ENTRY_10(23);
            CUSTOM_TYPE_ENTRY_10(24);
            CUSTOM_TYPE_ENTRY_10(25);
            CUSTOM_TYPE_ENTRY_10(26);
            CUSTOM_TYPE_ENTRY_10(27);
            CUSTOM_TYPE_ENTRY_10(28);
            CUSTOM_TYPE_ENTRY_10(29);
            CUSTOM_TYPE_ENTRY_10(30);
        }
    }

    qScriptRegisterMetaType_helper(this, type, internalMarshalFunc, internalDemarshalFunc, QScriptValue());
}

Q_DECLARE_METATYPE(ScriptValue);

static QScriptValue ScriptValueToQScriptValue(QScriptEngine* engine, const ScriptValue& src) {
    return ScriptValueQtWrapper::fullUnwrap(static_cast<ScriptEngineQtScript*>(engine), src);
}

static void ScriptValueFromQScriptValue(const QScriptValue& src, ScriptValue& dest) {
    ScriptEngineQtScript* engine = static_cast<ScriptEngineQtScript*>(src.engine());
    dest = ScriptValue(new ScriptValueQtWrapper(engine, src));
}

static ScriptValue StringListToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QStringList& src = *reinterpret_cast<const QStringList*>(pSrc);
    int len = src.length();
    ScriptValue dest = engine->newArray(len);
    for (int idx = 0; idx < len; ++idx) {
        dest.setProperty(idx, engine->newValue(src.at(idx)));
    }
    return dest;
}

static bool StringListFromScriptValue(const ScriptValue& src, void* pDest) {
    if(!src.isArray()) return false;
    QStringList& dest = *reinterpret_cast<QStringList*>(pDest);
    int len = src.property("length").toInteger();
    dest.clear();
    for (int idx = 0; idx < len; ++idx) {
        dest.append(src.property(idx).toString());
    }
    return true;
}

static ScriptValue VariantListToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QVariantList& src = *reinterpret_cast<const QVariantList*>(pSrc);
    int len = src.length();
    ScriptValue dest = engine->newArray(len);
    for (int idx = 0; idx < len; ++idx) {
        dest.setProperty(idx, engine->newVariant(src.at(idx)));
    }
    return dest;
}

static bool VariantListFromScriptValue(const ScriptValue& src, void* pDest) {
    if(!src.isArray()) return false;
    QVariantList& dest = *reinterpret_cast<QVariantList*>(pDest);
    int len = src.property("length").toInteger();
    dest.clear();
    for (int idx = 0; idx < len; ++idx) {
        dest.append(src.property(idx).toVariant());
    }
    return true;
}

static ScriptValue VariantMapToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QVariantMap& src = *reinterpret_cast<const QVariantMap*>(pSrc);
    ScriptValue dest = engine->newObject();
    for (QVariantMap::const_iterator iter = src.cbegin(); iter != src.cend(); ++iter) {
        dest.setProperty(iter.key(), engine->newVariant(iter.value()));
    }
    return dest;
}

static bool VariantMapFromScriptValue(const ScriptValue& src, void* pDest) {
    QVariantMap& dest = *reinterpret_cast<QVariantMap*>(pDest);
    dest.clear();
    ScriptValueIteratorPointer iter = src.newIterator();
    while (iter->hasNext()) {
        iter->next();
        dest.insert(iter->name(), iter->value().toVariant());
    }
    return true;
}

static ScriptValue VariantHashToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QVariantHash& src = *reinterpret_cast<const QVariantHash*>(pSrc);
    ScriptValue dest = engine->newObject();
    for (QVariantHash::const_iterator iter = src.cbegin(); iter != src.cend(); ++iter) {
        dest.setProperty(iter.key(), engine->newVariant(iter.value()));
    }
    return dest;
}

static bool VariantHashFromScriptValue(const ScriptValue& src, void* pDest) {
    QVariantHash& dest = *reinterpret_cast<QVariantHash*>(pDest);
    dest.clear();
    ScriptValueIteratorPointer iter = src.newIterator();
    while (iter->hasNext()) {
        iter->next();
        dest.insert(iter->name(), iter->value().toVariant());
    }
    return true;
}

static ScriptValue JsonValueToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QJsonValue& src = *reinterpret_cast<const QJsonValue*>(pSrc);
    return engine->newVariant(src.toVariant());
}

static bool JsonValueFromScriptValue(const ScriptValue& src, void* pDest) {
    QJsonValue& dest = *reinterpret_cast<QJsonValue*>(pDest);
    dest = QJsonValue::fromVariant(src.toVariant());
    return true;
}

static ScriptValue JsonObjectToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QJsonObject& src = *reinterpret_cast<const QJsonObject*>(pSrc);
    QVariantMap map = src.toVariantMap();
    ScriptValue dest = engine->newObject();
    for (QVariantMap::const_iterator iter = map.cbegin(); iter != map.cend(); ++iter) {
        dest.setProperty(iter.key(), engine->newVariant(iter.value()));
    }
    return dest;
}

static bool JsonObjectFromScriptValue(const ScriptValue& src, void* pDest) {
    QJsonObject& dest = *reinterpret_cast<QJsonObject*>(pDest);
    QVariantMap map;
    ScriptValueIteratorPointer iter = src.newIterator();
    while (iter->hasNext()) {
        iter->next();
        map.insert(iter->name(), iter->value().toVariant());
    }
    dest = QJsonObject::fromVariantMap(map);
    return true;
}

static ScriptValue JsonArrayToScriptValue(ScriptEngine* engine, const void* pSrc) {
    const QJsonArray& src = *reinterpret_cast<const QJsonArray*>(pSrc);
    QVariantList list = src.toVariantList();
    int len = list.length();
    ScriptValue dest = engine->newArray(len);
    for (int idx = 0; idx < len; ++idx) {
        dest.setProperty(idx, engine->newVariant(list.at(idx)));
    }
    return dest;
}

static bool JsonArrayFromScriptValue(const ScriptValue& src, void* pDest) {
    if(!src.isArray()) return false;
    QJsonArray& dest = *reinterpret_cast<QJsonArray*>(pDest);
    QVariantList list;
    int len = src.property("length").toInteger();
    for (int idx = 0; idx < len; ++idx) {
        list.append(src.property(idx).toVariant());
    }
    dest = QJsonArray::fromVariantList(list);
    return true;
}

// QMetaType::QJsonArray

void ScriptEngineQtScript::registerSystemTypes() {
    qScriptRegisterMetaType(this, ScriptValueToQScriptValue, ScriptValueFromQScriptValue);

    QMutexLocker guard(&_customTypeProtect);

    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QStringList, CustomMarshal()).value();
        customType.demarshalFunc = StringListFromScriptValue;
        customType.marshalFunc = StringListToScriptValue;
    }
    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QVariantList, CustomMarshal()).value();
        customType.demarshalFunc = VariantListFromScriptValue;
        customType.marshalFunc = VariantListToScriptValue;
    }
    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QVariantMap, CustomMarshal()).value();
        customType.demarshalFunc = VariantMapFromScriptValue;
        customType.marshalFunc = VariantMapToScriptValue;
    }
    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QVariantHash, CustomMarshal()).value();
        customType.demarshalFunc = VariantHashFromScriptValue;
        customType.marshalFunc = VariantHashToScriptValue;
    }
    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QJsonValue, CustomMarshal()).value();
        customType.demarshalFunc = JsonValueFromScriptValue;
        customType.marshalFunc = JsonValueToScriptValue;
    }
    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QJsonObject, CustomMarshal()).value();
        customType.demarshalFunc = JsonObjectFromScriptValue;
        customType.marshalFunc = JsonObjectToScriptValue;
    }
    {
        CustomMarshal& customType = _customTypes.insert(QMetaType::QJsonArray, CustomMarshal()).value();
        customType.demarshalFunc = JsonArrayFromScriptValue;
        customType.marshalFunc = JsonArrayToScriptValue;
    }
}

bool ScriptEngineQtScript::castValueToVariant(const QScriptValue& val, QVariant& dest, int destType) {

    // if we're not particularly interested in a specific type, try to detect if we're dealing with a registered type
    if (destType == QMetaType::UnknownType) {
        QObject* obj = ScriptObjectQtProxy::unwrap(val);
        if (obj) {
            for (const QMetaObject* metaObject = obj->metaObject(); metaObject; metaObject = metaObject->superClass()) {
                QByteArray typeName = QByteArray(metaObject->className()) + "*";
                int typeId = QMetaType::type(typeName.constData());
                if (typeId != QMetaType::UnknownType) {
                    destType = typeId;
                    break;
                }
            }
        }
    }

    if (destType == qMetaTypeId<ScriptValue>()) {
        dest = QVariant::fromValue(ScriptValue(new ScriptValueQtWrapper(this, val)));
        return true;
    }

    // do we have a registered handler for this type?
    ScriptEngine::DemarshalFunction demarshalFunc = nullptr;
    {
        QMutexLocker guard(&_customTypeProtect);
        CustomMarshalMap::const_iterator lookup = _customTypes.find(destType);
        if (lookup != _customTypes.cend()) {
            demarshalFunc = lookup.value().demarshalFunc;
        }
    }
    if (demarshalFunc) {
        void* destStorage = QMetaType::create(destType);
        ScriptValue wrappedVal(new ScriptValueQtWrapper(this, val));
        bool success = demarshalFunc(wrappedVal, destStorage);
        dest = success ? QVariant(destType, destStorage) : QVariant();
        QMetaType::destroy(destType, destStorage);
        return success;
    } else {
        switch (destType) {
            case QMetaType::UnknownType:
                if (val.isUndefined()) {
                    dest = QVariant();
                    break;
                }
                if (val.isNull()) {
                    dest = QVariant::fromValue(nullptr);
                    break;
                }
                if (val.isBool()) {
                    dest = QVariant::fromValue(val.toBool());
                    break;
                }
                if (val.isString()) {
                    dest = QVariant::fromValue(val.toString());
                    break;
                }
                if (val.isNumber()) {
                    dest = QVariant::fromValue(val.toNumber());
                    break;
                }
                {
                    QObject* obj = ScriptObjectQtProxy::unwrap(val);
                    if (obj) {
                        dest = QVariant::fromValue(obj);
                        break;
                    }
                }
                {
                    QVariant var = ScriptVariantQtProxy::unwrap(val);
                    if (var.isValid()) {
                        dest = var;
                        break;
                    }
                }
                dest = val.toVariant();
                break;
            case QMetaType::Bool:
                dest = QVariant::fromValue(val.toBool());
                break;
            case QMetaType::QDateTime:
            case QMetaType::QDate:
                Q_ASSERT(val.isDate());
                dest = QVariant::fromValue(val.toDateTime());
                break;
            case QMetaType::UInt:
            case QMetaType::ULong:
                dest = QVariant::fromValue(val.toUInt32());
                break;
            case QMetaType::Int:
            case QMetaType::Long:
            case QMetaType::Short:
                dest = QVariant::fromValue(val.toInt32());
                break;
            case QMetaType::Double:
            case QMetaType::Float:
            case QMetaType::ULongLong:
            case QMetaType::LongLong:
                dest = QVariant::fromValue(val.toNumber());
                break;
            case QMetaType::QString:
            case QMetaType::QByteArray:
                dest = QVariant::fromValue(val.toString());
                break;
            case QMetaType::UShort:
                dest = QVariant::fromValue(val.toUInt16());
                break;
            case QMetaType::QObjectStar:
                dest = QVariant::fromValue(ScriptObjectQtProxy::unwrap(val));
                break;
            default:
                // check to see if this is a pointer to a QObject-derived object
                if (QMetaType::typeFlags(destType) & QMetaType::PointerToQObject) {
                    dest = QVariant::fromValue(ScriptObjectQtProxy::unwrap(val));
                    break;
                }
                // check to see if we have a registered prototype
                {
                    QVariant var = ScriptVariantQtProxy::unwrap(val);
                    if (var.isValid()) {
                        dest = var;
                        break;
                    }
                }
                // last chance, just convert it to a variant
                dest = val.toVariant();
                break;
        }
    }

    return destType == QMetaType::UnknownType || dest.userType() == destType || dest.convert(destType);
}

QScriptValue ScriptEngineQtScript::castVariantToValue(const QVariant& val) {
    int valTypeId = val.userType();

    if (valTypeId == qMetaTypeId<ScriptValue>()) {
        // this is a wrapped ScriptValue, so just unwrap it and call it good
        ScriptValue innerVal = val.value<ScriptValue>();
        return ScriptValueQtWrapper::fullUnwrap(this, innerVal);
    }

    // do we have a registered handler for this type?
    ScriptEngine::MarshalFunction marshalFunc = nullptr;
    {
        QMutexLocker guard(&_customTypeProtect);
        CustomMarshalMap::const_iterator lookup = _customTypes.find(valTypeId);
        if (lookup != _customTypes.cend()) {
            marshalFunc = lookup.value().marshalFunc;
        }
    }
    if (marshalFunc) {
        ScriptValue wrappedVal = marshalFunc(this, val.constData());
        return ScriptValueQtWrapper::fullUnwrap(this, wrappedVal);
    }

    switch (valTypeId) {
        case QMetaType::UnknownType:
        case QMetaType::Void:
            return QScriptValue(this, QScriptValue::UndefinedValue);
        case QMetaType::Nullptr:
            return QScriptValue(this, QScriptValue::NullValue);
        case QMetaType::Bool:
            return QScriptValue(this, val.toBool());
        case QMetaType::Int:
        case QMetaType::Long:
        case QMetaType::Short:
            return QScriptValue(this, val.toInt());
        case QMetaType::UInt:
        case QMetaType::ULong:
        case QMetaType::UShort:
            return QScriptValue(this, val.toUInt());
        case QMetaType::Float:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Double:
            return QScriptValue(this, val.toFloat());
        case QMetaType::QString:
        case QMetaType::QByteArray:
            return QScriptValue(this, val.toString());
        case QMetaType::QVariant:
            return castVariantToValue(val.value<QVariant>());
        case QMetaType::QObjectStar:
            return ScriptObjectQtProxy::newQObject(this, val.value<QObject*>(), ScriptEngine::QtOwnership);
        case QMetaType::QDateTime:
            return static_cast<QScriptEngine*>(this)->newDate(val.value<QDateTime>());
        case QMetaType::QDate:
            return static_cast<QScriptEngine*>(this)->newDate(val.value<QDate>().startOfDay());
        default:
            // check to see if this is a pointer to a QObject-derived object
            if (QMetaType::typeFlags(valTypeId) & QMetaType::PointerToQObject) {
                return ScriptObjectQtProxy::newQObject(this, val.value<QObject*>(), ScriptEngine::QtOwnership);
            }
            // have we set a prototype'd variant?
            {
                QMutexLocker guard(&_customTypeProtect);
                CustomPrototypeMap::const_iterator lookup = _customPrototypes.find(valTypeId);
                if (lookup != _customPrototypes.cend()) {
                    return ScriptVariantQtProxy::newVariant(this, val, lookup.value());
                }
            }
            // just do a generic variant
            return QScriptEngine::newVariant(val);
    }
}