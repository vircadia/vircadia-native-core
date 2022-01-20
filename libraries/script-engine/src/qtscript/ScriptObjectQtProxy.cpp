//
//  ScriptObjectQtProxy.cpp
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 12/5/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptObjectQtProxy.h"

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include <QtScript/QScriptContext>

#include "ScriptContextQtWrapper.h"
#include "ScriptValueQtWrapper.h"

Q_DECLARE_METATYPE(QScriptContext*)
Q_DECLARE_METATYPE(ScriptValue)
Q_DECLARE_METATYPE(QScriptValue)

Q_DECLARE_METATYPE(QSharedPointer<ScriptObjectQtProxy>)
Q_DECLARE_METATYPE(QSharedPointer<ScriptVariantQtProxy>)

// Used strictly to replace the "this" object value for property access.  May expand to a full context element
// if we find it necessary to, but hopefully not needed
class ScriptPropertyContextQtWrapper final : public ScriptContext {
public:  // construction
    inline ScriptPropertyContextQtWrapper(const ScriptValue& object, ScriptContext* parentContext) :
        _parent(parentContext), _object(object) {}

public:  // ScriptContext implementation
    virtual int argumentCount() const override { return _parent->argumentCount(); }
    virtual ScriptValue argument(int index) const override { return _parent->argument(index); }
    virtual QStringList backtrace() const override { return _parent->backtrace(); }
    virtual ScriptValue callee() const override { return _parent->callee(); }
    virtual ScriptEnginePointer engine() const override { return _parent->engine(); }
    virtual ScriptFunctionContextPointer functionContext() const override { return _parent->functionContext(); }
    virtual ScriptContextPointer parentContext() const override { return _parent->parentContext(); }
    virtual ScriptValue thisObject() const override { return _object; }
    virtual ScriptValue throwError(const QString& text) override { return _parent->throwError(text); }
    virtual ScriptValue throwValue(const ScriptValue& value) override { return _parent->throwValue(value); }

private:  // storage
    ScriptContext* _parent;
    const ScriptValue& _object;
};

QScriptValue ScriptObjectQtProxy::newQObject(ScriptEngineQtScript* engine, QObject* object,
                                             ScriptEngine::ValueOwnership ownership,
                                             const ScriptEngine::QObjectWrapOptions& options) {
    QScriptEngine* qengine = static_cast<QScriptEngine*>(engine);

    // do we already have a valid wrapper for this QObject?
    {
        QMutexLocker guard(&engine->_qobjectWrapperMapProtect);
        ScriptEngineQtScript::ObjectWrapperMap::const_iterator lookup = engine->_qobjectWrapperMap.find(object);
        if (lookup != engine->_qobjectWrapperMap.end()) {
            QSharedPointer<ScriptObjectQtProxy> proxy = lookup.value().lock();
            if (proxy) return qengine->newObject(proxy.get(), qengine->newVariant(QVariant::fromValue(proxy)));;
        }
    }
    
    bool ownsObject;
    switch (ownership) {
        case ScriptEngine::QtOwnership:
            ownsObject = false;
            break;
        case ScriptEngine::ScriptOwnership:
            ownsObject = true;
            break;
        case ScriptEngine::AutoOwnership:
            ownsObject = !object->parent();
            break;
    }

    // create the wrapper
    auto proxy = QSharedPointer<ScriptObjectQtProxy>::create(engine, object, ownsObject, options);

    {
        QMutexLocker guard(&engine->_qobjectWrapperMapProtect);

        // check again to see if someone else created the wrapper while we were busy
        ScriptEngineQtScript::ObjectWrapperMap::const_iterator lookup = engine->_qobjectWrapperMap.find(object);
        if (lookup != engine->_qobjectWrapperMap.end()) {
            QSharedPointer<ScriptObjectQtProxy> proxy = lookup.value().lock();
            if (proxy) return qengine->newObject(proxy.get(), qengine->newVariant(QVariant::fromValue(proxy)));;
        }

        // register the wrapper with the engine and make sure it cleans itself up
        engine->_qobjectWrapperMap.insert(object, proxy);
        QPointer<ScriptEngineQtScript> enginePtr = engine;
        object->connect(object, &QObject::destroyed, engine, [enginePtr, object]() {
            if (!enginePtr) return;
            QMutexLocker guard(&enginePtr->_qobjectWrapperMapProtect);
            ScriptEngineQtScript::ObjectWrapperMap::iterator lookup = enginePtr->_qobjectWrapperMap.find(object);
            if (lookup != enginePtr->_qobjectWrapperMap.end()) {
                enginePtr->_qobjectWrapperMap.erase(lookup);
            }
        });
    }

    return qengine->newObject(proxy.get(), qengine->newVariant(QVariant::fromValue(proxy)));
}

ScriptObjectQtProxy* ScriptObjectQtProxy::unwrapProxy(const QScriptValue& val) {
    QScriptClass* scriptClass = val.scriptClass();
    return scriptClass ? dynamic_cast<ScriptObjectQtProxy*>(scriptClass) : nullptr;
}

QObject* ScriptObjectQtProxy::unwrap(const QScriptValue& val) {
    if (val.isQObject()) {
        return val.toQObject();
    }
    ScriptObjectQtProxy* proxy = unwrapProxy(val);
    return proxy ? proxy->toQtValue() : nullptr;
}

ScriptObjectQtProxy::~ScriptObjectQtProxy() {
    if (_ownsObject) {
        QObject* qobject = _object;
        if(qobject) qobject->deleteLater();
    }
}

void ScriptObjectQtProxy::investigate() {
    QObject* qobject = _object;
    Q_ASSERT(qobject);
    if (!qobject) return;

    const QMetaObject* metaObject = qobject->metaObject();

    // discover properties
    int startIdx = _wrapOptions & ScriptEngine::ExcludeSuperClassProperties ? metaObject->propertyOffset() : 0;
    int num = metaObject->propertyCount();
    for (int idx = startIdx; idx < num; ++idx) {
        QMetaProperty prop = metaObject->property(idx);
        if (!prop.isScriptable()) continue;

        // always exclude child objects (at least until we decide otherwise)
        int metaTypeId = prop.userType();
        if (metaTypeId != QMetaType::UnknownType) {
            QMetaType metaType(metaTypeId);
            if (metaType.flags() & QMetaType::PointerToQObject) {
                continue;
            }
        }

        PropertyDef& propDef = _props.insert(idx, PropertyDef()).value();
        propDef.name = _engine->toStringHandle(QString::fromLatin1(prop.name()));
        propDef.flags = QScriptValue::Undeletable | QScriptValue::PropertyGetter | QScriptValue::PropertySetter |
                        QScriptValue::QObjectMember;
        if (prop.isConstant()) propDef.flags |= QScriptValue::ReadOnly;
    }

    // discover methods
    startIdx = _wrapOptions & ScriptEngine::ExcludeSuperClassMethods ? metaObject->methodCount() : 0;
    num = metaObject->methodCount();
    QHash<QScriptString, int> methodNames;
    for (int idx = startIdx; idx < num; ++idx) {
        QMetaMethod method = metaObject->method(idx);
        
        // perhaps keep this comment?  Calls (like AudioScriptingInterface::playSound) seem to expect non-public methods to be script-accessible
        /* if (method.access() != QMetaMethod::Public) continue;*/

        bool isSignal = false;
        QByteArray szName = method.name();

        switch (method.methodType()) {
            case QMetaMethod::Constructor:
                continue;
            case QMetaMethod::Signal:
                isSignal = true;
                break;
            case QMetaMethod::Slot:
                if (_wrapOptions & ScriptEngine::ExcludeSlots) {
                    continue;
                }
                if (szName == "deleteLater") {
                    continue;
                }
                break;
        }

        QScriptString name = _engine->toStringHandle(QString::fromLatin1(szName));
        auto nameLookup = methodNames.find(name);
        if (isSignal) {
            if (nameLookup == methodNames.end()) {
                SignalDef& signalDef = _signals.insert(idx, SignalDef()).value();
                signalDef.name = name;
                signalDef.signal = method;
                methodNames.insert(name, idx);
            } else {
                int originalMethodId = nameLookup.value();
                SignalDefMap::iterator signalLookup = _signals.find(originalMethodId);
                Q_ASSERT(signalLookup != _signals.end());
                SignalDef& signalDef = signalLookup.value();
                Q_ASSERT(signalDef.signal.parameterCount() != method.parameterCount());
                if (signalDef.signal.parameterCount() < method.parameterCount()) {
                    signalDef.signal = method;
                }
            }
        } else {
            if (nameLookup == methodNames.end()) {
                MethodDef& methodDef = _methods.insert(idx, MethodDef()).value();
                methodDef.name = name;
                methodDef.methods.append(method);
                methodNames.insert(name, idx);
            } else {
                int originalMethodId = nameLookup.value();
                MethodDefMap::iterator methodLookup = _methods.find(originalMethodId);
                Q_ASSERT(methodLookup != _methods.end());
                MethodDef& methodDef = methodLookup.value();
                methodDef.methods.append(method);
            }
        }
    }
}

QString ScriptObjectQtProxy::name() const {
    Q_ASSERT(_object);
    if (!_object) return "";
    return _object ? _object->objectName() : "";
    QString objectName = _object->objectName();
    if (!objectName.isEmpty()) return objectName;
    return _object->metaObject()->className();
}

QScriptClass::QueryFlags ScriptObjectQtProxy::queryProperty(const QScriptValue& object, const QScriptString& name, QueryFlags flags, uint* id) {
    // check for properties
    for (PropertyDefMap::const_iterator trans = _props.cbegin(); trans != _props.cend(); ++trans) {
        const PropertyDef& propDef = trans.value();
        if (propDef.name != name) continue;
        *id = trans.key() | PROPERTY_TYPE;
        return flags & (HandlesReadAccess | HandlesWriteAccess);
    }

    // check for methods
    for (MethodDefMap::const_iterator trans = _methods.cbegin(); trans != _methods.cend(); ++trans) {
        if (trans.value().name != name) continue;
        *id = trans.key() | METHOD_TYPE;
        return flags & (HandlesReadAccess | HandlesWriteAccess);
    }

    // check for signals
    for (SignalDefMap::const_iterator trans = _signals.cbegin(); trans != _signals.cend(); ++trans) {
        if (trans.value().name != name) continue;
        *id = trans.key() | SIGNAL_TYPE;
        return flags & (HandlesReadAccess | HandlesWriteAccess);
    }

    return QueryFlags();
}

QScriptValue::PropertyFlags ScriptObjectQtProxy::propertyFlags(const QScriptValue& object, const QScriptString& name, uint id) {
    QObject* qobject = _object;
    if (!qobject) {
        return QScriptValue::PropertyFlags();
    }

    switch (id & TYPE_MASK) {
        case PROPERTY_TYPE: {
            PropertyDefMap::const_iterator lookup = _props.find(id & ~TYPE_MASK);
            if (lookup == _props.cend()) return QScriptValue::PropertyFlags();
            const PropertyDef& propDef = lookup.value();
            return propDef.flags;
        }
        case METHOD_TYPE: {
            MethodDefMap::const_iterator lookup = _methods.find(id & ~TYPE_MASK);
            if (lookup == _methods.cend()) return QScriptValue::PropertyFlags();
            return QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::QObjectMember;
        }
        case SIGNAL_TYPE: {
            SignalDefMap::const_iterator lookup = _signals.find(id & ~TYPE_MASK);
            if (lookup == _signals.cend()) return QScriptValue::PropertyFlags();
            return QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::QObjectMember;
        }
    }
    return QScriptValue::PropertyFlags();
}

QScriptValue ScriptObjectQtProxy::property(const QScriptValue& object, const QScriptString& name, uint id) {
    QObject* qobject = _object;
    if (!qobject) {
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::ReferenceError, "Referencing deleted native object");
        return QScriptValue();
    }

    const QMetaObject* metaObject = qobject->metaObject();

    switch (id & TYPE_MASK) {
        case PROPERTY_TYPE: {
            int propId = id & ~TYPE_MASK;
            PropertyDefMap::const_iterator lookup = _props.find(propId);
            if (lookup == _props.cend()) return QScriptValue();
            const PropertyDef& propDef = lookup.value();

            QMetaProperty prop = metaObject->property(propId);
            ScriptValue scriptThis = ScriptValue(new ScriptValueQtWrapper(_engine, object));
            ScriptPropertyContextQtWrapper ourContext(scriptThis, _engine->currentContext());
            ScriptContextGuard guard(&ourContext);

            QVariant varValue = prop.read(qobject);
            return _engine->castVariantToValue(varValue);
        }
        case METHOD_TYPE: {
            int methodId = id & ~TYPE_MASK;
            MethodDefMap::const_iterator lookup = _methods.find(methodId);
            if (lookup == _methods.cend()) return QScriptValue();
            return static_cast<QScriptEngine*>(_engine)->newObject(
                new ScriptMethodQtProxy(_engine, qobject, object, lookup.value().methods));
        }
        case SIGNAL_TYPE: {
            int signalId = id & ~TYPE_MASK;
            SignalDefMap::const_iterator defLookup = _signals.find(signalId);
            if (defLookup == _signals.cend()) return QScriptValue();

            InstanceMap::const_iterator instLookup = _signalInstances.find(signalId);
            if (instLookup == _signalInstances.cend() || instLookup.value().isNull()) {
                instLookup = _signalInstances.insert(signalId,
                    new ScriptSignalQtProxy(_engine, qobject, object, defLookup.value().signal));
                Q_ASSERT(instLookup != _signalInstances.cend());
            }
            ScriptSignalQtProxy* proxy = instLookup.value();

            QScriptEngine::QObjectWrapOptions options = QScriptEngine::ExcludeSuperClassContents |
                                                        QScriptEngine::ExcludeDeleteLater |
                                                        QScriptEngine::PreferExistingWrapperObject;
            return static_cast<QScriptEngine*>(_engine)->newQObject(proxy, QScriptEngine::ScriptOwnership, options);
        }
    }
    return QScriptValue();
}

void ScriptObjectQtProxy::setProperty(QScriptValue& object, const QScriptString& name, uint id, const QScriptValue& value) {
    if (!(id & PROPERTY_TYPE)) return;
    QObject* qobject = _object;
    if (!qobject) {
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::ReferenceError, "Referencing deleted native object");
        return;
    }

    int propId = id & ~TYPE_MASK;
    PropertyDefMap::const_iterator lookup = _props.find(propId);
    if (lookup == _props.cend()) return;
    const PropertyDef& propDef = lookup.value();
    if (propDef.flags & QScriptValue::ReadOnly) return;

    const QMetaObject* metaObject = qobject->metaObject();
    QMetaProperty prop = metaObject->property(propId);

    ScriptValue scriptThis = ScriptValue(new ScriptValueQtWrapper(_engine, object));
    ScriptPropertyContextQtWrapper ourContext(scriptThis, _engine->currentContext());
    ScriptContextGuard guard(&ourContext);

    int propTypeId = prop.userType();
    Q_ASSERT(propTypeId != QMetaType::UnknownType);
    QVariant varValue;
    if(!_engine->castValueToVariant(value, varValue, propTypeId)) {
        QByteArray propTypeName = QMetaType(propTypeId).name();
        QByteArray valTypeName = _engine->valueType(value).toLatin1();
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::TypeError, QString("Cannot convert %1 to %2").arg(valTypeName, propTypeName));
        return;
    }
    prop.write(qobject, varValue);
}

ScriptVariantQtProxy::ScriptVariantQtProxy(ScriptEngineQtScript* engine, const QVariant& variant, QScriptValue scriptProto, ScriptObjectQtProxy* proto) :
    QScriptClass(engine), _engine(engine), _variant(variant), _scriptProto(scriptProto), _proto(proto) {
    _name = QString::fromLatin1(variant.typeName());
}

QScriptValue ScriptVariantQtProxy::newVariant(ScriptEngineQtScript* engine, const QVariant& variant, QScriptValue proto) {
    QScriptEngine* qengine = static_cast<QScriptEngine*>(engine);
    ScriptObjectQtProxy* protoProxy = ScriptObjectQtProxy::unwrapProxy(proto);
    if (!protoProxy) {
        Q_ASSERT(protoProxy);
        return qengine->newVariant(variant);
    }
    auto proxy = QSharedPointer<ScriptVariantQtProxy>::create(engine, variant, proto, protoProxy);
    return qengine->newObject(proxy.get(), qengine->newVariant(QVariant::fromValue(proxy)));
}

ScriptVariantQtProxy* ScriptVariantQtProxy::unwrapProxy(const QScriptValue& val) {
    QScriptClass* scriptClass = val.scriptClass();
    return scriptClass ? dynamic_cast<ScriptVariantQtProxy*>(scriptClass) : nullptr;
}

QVariant ScriptVariantQtProxy::unwrap(const QScriptValue& val) {
    ScriptVariantQtProxy* proxy = unwrapProxy(val);
    return proxy ? proxy->toQtValue() : QVariant();
}

QString ScriptMethodQtProxy::fullName() const {
    Q_ASSERT(_object);
    if (!_object) return "";
    Q_ASSERT(!_metas.isEmpty());
    const QMetaMethod& firstMethod = _metas.front();
    QString objectName = _object->objectName();
    if (!objectName.isEmpty()) {
        return QString("%1.%2").arg(objectName, firstMethod.name());
    }
    return QString("%1::%2").arg(_object->metaObject()->className(), firstMethod.name());
}

bool ScriptMethodQtProxy::supportsExtension(Extension extension) const {
    switch (extension) {
        case Callable:
            return true;
        default:
            return false;
    }
}

QVariant ScriptMethodQtProxy::extension(Extension extension, const QVariant& argument) {
    if (extension != Callable) return QVariant();
    QScriptContext* context = qvariant_cast<QScriptContext*>(argument);

    QObject* qobject = _object;
    if (!qobject) {
        context->throwError(QScriptContext::ReferenceError, "Referencing deleted native object");
        return QVariant();
    }

    int scriptNumArgs = context->argumentCount();
    Q_ASSERT(scriptNumArgs < 10);
    int numArgs = std::min(scriptNumArgs, 10);
    
    const int scriptValueTypeId = qMetaTypeId<ScriptValue>();

    int parameterConversionFailureId = 0;
    int parameterConversionFailureCount = 0;

    for (auto iter = _metas.cbegin(); iter != _metas.end(); ++iter) {
        const QMetaMethod& meta = *iter;
        int methodNumArgs = meta.parameterCount();
        if (methodNumArgs != numArgs) {
            continue;
        }

        QList<ScriptValue> qScriptArgList;
        QList<QVariant> qVarArgList;
        QGenericArgument qGenArgs[10];
        int conversionFailures = 0;
        for (int arg = 0; arg < numArgs; ++arg) {
            int methodArgTypeId = meta.parameterType(arg);
            Q_ASSERT(methodArgTypeId != QMetaType::UnknownType);
            QScriptValue argVal = context->argument(arg);
            if (methodArgTypeId == scriptValueTypeId) {
                qScriptArgList.append(ScriptValue(new ScriptValueQtWrapper(_engine, argVal)));
                qGenArgs[arg] = Q_ARG(ScriptValue, qScriptArgList.back());
            } else if (methodArgTypeId == QMetaType::QVariant) {
                qVarArgList.append(argVal.toVariant());
                qGenArgs[arg] = Q_ARG(QVariant, qVarArgList.back());
            } else {
                QVariant varArgVal;
                if (!_engine->castValueToVariant(argVal, varArgVal, methodArgTypeId)) {
                    conversionFailures++;
                } else {
                    qVarArgList.append(varArgVal);
                    const QVariant& converted = qVarArgList.back();

                    // a lot of type conversion assistance thanks to https://stackoverflow.com/questions/28457819/qt-invoke-method-with-qvariant
                    // A const_cast is needed because calling data() would detach the QVariant.
                    qGenArgs[arg] =
                        QGenericArgument(QMetaType::typeName(converted.userType()), const_cast<void*>(converted.constData()));
                }
            }
        }
        if (conversionFailures) {
            if (conversionFailures < parameterConversionFailureCount || !parameterConversionFailureCount) {
                parameterConversionFailureCount = conversionFailures;
                parameterConversionFailureId = meta.methodIndex();
            }
            continue;
        }

        ScriptContextQtWrapper ourContext(_engine, context);
        ScriptContextGuard guard(&ourContext);

        int returnTypeId = meta.returnType();

        // The Qt MOC engine will automatically call qRegisterMetaType on invokable parameters and properties, but there's
        // nothing in there for return values so these need to be explicitly runtime-registered!
        Q_ASSERT(returnTypeId != QMetaType::UnknownType);
        if (returnTypeId == QMetaType::UnknownType) {
            context->throwError(QString("Cannot call native function %1, its return value has not been registered with Qt").arg(fullName()));
            return QVariant();
        } else if (returnTypeId == QMetaType::Void) {
            bool success = meta.invoke(qobject, Qt::DirectConnection, qGenArgs[0], qGenArgs[1], qGenArgs[2], qGenArgs[3],
                                        qGenArgs[4], qGenArgs[5], qGenArgs[6], qGenArgs[7], qGenArgs[8], qGenArgs[9]);
            if (!success) {
                context->throwError(QString("Unexpected: Native call of %1 failed").arg(fullName()));
            }
            return QVariant();
        } else if (returnTypeId == scriptValueTypeId) {
            ScriptValue result;
            bool success = meta.invoke(qobject, Qt::DirectConnection, Q_RETURN_ARG(ScriptValue, result), qGenArgs[0],
                                        qGenArgs[1], qGenArgs[2], qGenArgs[3], qGenArgs[4], qGenArgs[5], qGenArgs[6],
                                        qGenArgs[7], qGenArgs[8], qGenArgs[9]);
            if (!success) {
                context->throwError(QString("Unexpected: Native call of %1 failed").arg(fullName()));
                return QVariant();
            }
            QScriptValue qResult = ScriptValueQtWrapper::fullUnwrap(_engine, result);
            return QVariant::fromValue(qResult);
        } else {
            // a lot of type conversion assistance thanks to https://stackoverflow.com/questions/28457819/qt-invoke-method-with-qvariant
            const char* typeName = meta.typeName();
            QVariant qRetVal(returnTypeId, static_cast<void*>(NULL));
            QGenericReturnArgument sRetVal(typeName, const_cast<void*>(qRetVal.constData()));

            bool success =
                meta.invoke(qobject, Qt::DirectConnection, sRetVal, qGenArgs[0], qGenArgs[1], qGenArgs[2], qGenArgs[3],
                            qGenArgs[4], qGenArgs[5], qGenArgs[6], qGenArgs[7], qGenArgs[8], qGenArgs[9]);
            if (!success) {
                context->throwError(QString("Unexpected: Native call of %1 failed").arg(fullName()));
                return QVariant();
            }
            QScriptValue qResult = _engine->castVariantToValue(qRetVal);
            return QVariant::fromValue(qResult);
        }
    }

    // we failed to convert the call to C++, try to create a somewhat sane error message
    if (parameterConversionFailureCount == 0) {
        context->throwError(QString("Native call of %1 failed: unexpected parameter count").arg(fullName()));
        return QVariant();
    }

    const QMetaMethod& meta = _object->metaObject()->method(parameterConversionFailureId);
    int methodNumArgs = meta.parameterCount();
    Q_ASSERT(methodNumArgs == numArgs);

    for (int arg = 0; arg < numArgs; ++arg) {
        int methodArgTypeId = meta.parameterType(arg);
        Q_ASSERT(methodArgTypeId != QMetaType::UnknownType);
        QScriptValue argVal = context->argument(arg);
        if (methodArgTypeId != scriptValueTypeId && methodArgTypeId != QMetaType::QVariant) {
            QVariant varArgVal;
            if (!_engine->castValueToVariant(argVal, varArgVal, methodArgTypeId)) {
                QByteArray methodTypeName = QMetaType(methodArgTypeId).name();
                QByteArray argTypeName = _engine->valueType(argVal).toLatin1();
                context->throwError(QScriptContext::TypeError, QString("Native call of %1 failed: Cannot convert parameter %2 from %3 to %4")
                                                                   .arg(fullName()).arg(arg+1).arg(argTypeName, methodTypeName));
                return QVariant();
            }
        }
    }

    Q_ASSERT(false); // really shouldn't have gotten here -- it didn't work before and it's working now?
    return QVariant();
    context->throwError(QString("Native call of %1 failed: could not locate an overload with the requested arguments").arg(fullName()));
}

QString ScriptSignalQtProxy::fullName() const {
    Q_ASSERT(_object);
    if (!_object) return "";
    QString objectName = _object->objectName();
    if (!objectName.isEmpty()) {
        return QString("%1.%2").arg(objectName, _meta.name());
    }
    return QString("%1::%2").arg(_object->metaObject()->className(), _meta.name());
}

// Adapted from https://doc.qt.io/archives/qq/qq16-dynamicqobject.html, for connecting to a signal without a compile-time definition for it
int ScriptSignalQtProxy::qt_metacall(QMetaObject::Call call, int id, void** arguments) {
    id = ScriptSignalQtProxyBase::qt_metacall(call, id, arguments);
    if (id != 0 || call != QMetaObject::InvokeMetaMethod) {
        return id;
    }

    QScriptValueList args;
    int numArgs = _meta.parameterCount();
    for (int arg = 0; arg < numArgs; ++arg) {
        int methodArgTypeId = _meta.parameterType(arg);
        Q_ASSERT(methodArgTypeId != QMetaType::UnknownType);
        QVariant argValue(methodArgTypeId, arguments[arg+1]);
        args.append(_engine->castVariantToValue(argValue));
    }

    for (ConnectionList::iterator iter = _connections.begin(); iter != _connections.end(); ++iter) {
        Connection& conn = *iter;
        conn.callback.call(conn.thisValue, args);
    }

    return -1;
}

int ScriptSignalQtProxy::discoverMetaCallIdx() {
    const QMetaObject* ourMeta = metaObject();
    return ourMeta->methodCount();
}

ScriptSignalQtProxy::ConnectionList::iterator ScriptSignalQtProxy::findConnection(QScriptValue thisObject,
                                                                                  QScriptValue callback) {
    for (ConnectionList::iterator iter = _connections.begin(); iter != _connections.end(); ++iter) {
        Connection& conn = *iter;
        if (conn.callback.strictlyEquals(callback) && conn.thisValue.strictlyEquals(thisObject)) {
            return iter;
        }
    }
    return _connections.end();
}


void ScriptSignalQtProxy::connect(QScriptValue arg0, QScriptValue arg1) {
    QObject* qobject = _object;
    if (!qobject) {
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::ReferenceError, "Referencing deleted native object");
        return;
    }

    // untangle the arguments
    QScriptValue callback;
    QScriptValue callbackThis;
    if (arg1.isFunction()) {
        callbackThis = arg0;
        callback = arg1;
    } else {
        callback = arg0;
    }
    if (!callback.isFunction()) {
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::TypeError, "Function expected as argument to 'connect'");
        return;
    }

    // are we already connected?
    ConnectionList::iterator lookup = findConnection(callbackThis, callback);
    if (lookup != _connections.end()) {
        return; // already exists
    }

    // add a reference to ourselves to the destination callback
    QScriptValue destData = callback.data();
    Q_ASSERT(!destData.isValid() || destData.isArray());
    if (!destData.isArray()) {
        destData = static_cast<QScriptEngine*>(_engine)->newArray();
    }
    {
        QScriptValueList args;
        args << thisObject();
        destData.property("push").call(destData, args);
    }
    callback.setData(destData);

    // add this to our internal list of connections
    Connection newConn;
    newConn.callback = callback;
    newConn.thisValue = callbackThis;
    _connections.append(newConn);

    // inform Qt that we're connecting to this signal
    if (!_isConnected) {
        auto result = QMetaObject::connect(qobject, _meta.methodIndex(), this, _metaCallId);
        Q_ASSERT(result);
        _isConnected = true;
    }
}

void ScriptSignalQtProxy::disconnect(QScriptValue arg0, QScriptValue arg1) {
    QObject* qobject = _object;
    if (!qobject) {
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::ReferenceError, "Referencing deleted native object");
        return;
    }

    // untangle the arguments
    QScriptValue callback;
    QScriptValue callbackThis;
    if (arg1.isFunction()) {
        callbackThis = arg0;
        callback = arg1;
    } else {
        callback = arg0;
    }
    if (!callback.isFunction()) {
        QScriptContext* currentContext = static_cast<QScriptEngine*>(_engine)->currentContext();
        currentContext->throwError(QScriptContext::TypeError, "Function expected as argument to 'disconnect'");
        return;
    }

    // locate this connection in our list of connections
    ConnectionList::iterator lookup = findConnection(callbackThis, callback);
    if (lookup == _connections.end()) {
        return;  // not here
    }

    // remove it from our internal list of connections
    _connections.erase(lookup);

    // remove a reference to ourselves from the destination callback
    QScriptValue destData = callback.data();
    Q_ASSERT(destData.isArray());
    if (destData.isArray()) {
        QScriptValue qThis = thisObject();
        int len = destData.property("length").toInteger();
        bool foundIt = false;
        for (int idx = 0; idx < len && !foundIt; ++idx) {
            QScriptValue entry = destData.property(idx);
            if (entry.strictlyEquals(qThis)) {
                foundIt = true;
                QScriptValueList args;
                args << idx << 1;
                destData.property("splice").call(destData, args);
            }
        }
        Q_ASSERT(foundIt);
    }

    // inform Qt that we're no longer connected to this signal
    if (_connections.empty()) {
        Q_ASSERT(_isConnected);
        bool result = QMetaObject::disconnect(qobject, _meta.methodIndex(), this, _metaCallId);
        Q_ASSERT(result);
        _isConnected = false;
    }
}
