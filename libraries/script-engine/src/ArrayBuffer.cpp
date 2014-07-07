//
//  ArrayBuffer.cpp
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "ArrayBufferPrototype.h"

#include "ArrayBuffer.h"

Q_DECLARE_METATYPE(QByteArray*)
Q_DECLARE_METATYPE(ArrayBuffer*)

ArrayBuffer::ArrayBuffer(QScriptEngine* engine) : QObject(engine), QScriptClass(engine) {
    qScriptRegisterMetaType<QByteArray>(engine, toScriptValue, fromScriptValue);
    
    _byteLength = engine->toStringHandle(QLatin1String("byteLength"));
    
//    _proto = engine->newQObject(new ArrayBufferPrototype(this),
//                                QScriptEngine::QtOwnership,
//                                QScriptEngine::SkipMethodsInEnumeration
//                                | QScriptEngine::ExcludeSuperClassMethods
//                                | QScriptEngine::ExcludeSuperClassProperties);
    QScriptValue global = engine->globalObject();
    _proto.setPrototype(global.property("Object").property("prototype"));
    
    _ctor = engine->newFunction(construct, _proto);
    _ctor.setData(engine->toScriptValue(this));
}

QScriptValue ArrayBuffer::newInstance(unsigned long size) {
    engine()->reportAdditionalMemoryCost(size);
    QScriptValue data = engine()->newVariant(QVariant::fromValue(QByteArray(size, 0)));
    return engine()->newObject(this, data);
}

QScriptValue ArrayBuffer::newInstance(const QByteArray& ba) {
    QScriptValue data = engine()->newVariant(QVariant::fromValue(ba));
    return engine()->newObject(this, data);
}

QScriptValue ArrayBuffer::construct(QScriptContext* ctx, QScriptEngine* eng) {
    ArrayBuffer* cls = qscriptvalue_cast<ArrayBuffer*>(ctx->callee().data());
    QScriptValue arg = ctx->argument(0);
    int size = arg.toInt32();
    return cls ? cls->newInstance(size) : QScriptValue();
}

QScriptClass::QueryFlags ArrayBuffer::queryProperty(const QScriptValue& object,
                                                    const QScriptString& name,
                                                    QueryFlags flags, uint* id) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data());
    if (ba && name == _byteLength) {
        // if the property queried is byteLength, only handle read access
        return flags &= HandlesReadAccess;
    }
    return 0; // No access
}

QScriptValue ArrayBuffer::property(const QScriptValue &object,
                                   const QScriptString &name, uint id) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data());
    if (ba && name == _byteLength) {
        return ba->length();
    }
    return QScriptValue();
}

QScriptValue::PropertyFlags ArrayBuffer::propertyFlags(const QScriptValue& object,
                                                       const QScriptString& name, uint id) {
    return QScriptValue::Undeletable;
}

QString ArrayBuffer::name() const {
    return QLatin1String("ArrayBuffer");
}

QScriptValue ArrayBuffer::prototype() const {
    return _proto;
}

QScriptValue ArrayBuffer::constructor() const {
    return _ctor;
}

QScriptValue ArrayBuffer::toScriptValue(QScriptEngine* eng, const QByteArray& ba)
{
    QScriptValue ctor = eng->globalObject().property("ArrayBuffer");
    ArrayBuffer *cls = qscriptvalue_cast<ArrayBuffer*>(ctor.data());
    if (!cls) {
        return eng->newVariant(QVariant::fromValue(ba));
    }
    return cls->newInstance(ba);
}

void ArrayBuffer::fromScriptValue(const QScriptValue& obj, QByteArray& ba) {
    ba = qvariant_cast<QByteArray>(obj.data().toVariant());
}

