//
//  ArrayBufferClass.cpp
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "ArrayBufferPrototype.h"

#include "ArrayBufferClass.h"

static const QString CLASS_NAME = "ArrayBuffer";
static const QString BYTE_LENGTH_PROPERTY_NAME = "byteLength";

Q_DECLARE_METATYPE(QByteArray*)

ArrayBufferClass::ArrayBufferClass(QScriptEngine* engine) : QObject(engine), QScriptClass(engine) {
    qScriptRegisterMetaType<QByteArray>(engine, toScriptValue, fromScriptValue);
    QScriptValue global = engine->globalObject();
    
    // Save string handles for quick lookup
    _name = engine->toStringHandle(CLASS_NAME.toLatin1());
    _byteLength = engine->toStringHandle(BYTE_LENGTH_PROPERTY_NAME.toLatin1());
    
    // build prototype
    _proto = engine->newQObject(new ArrayBufferPrototype(this),
                                QScriptEngine::QtOwnership,
                                QScriptEngine::SkipMethodsInEnumeration |
                                QScriptEngine::ExcludeSuperClassMethods |
                                QScriptEngine::ExcludeSuperClassProperties);

    _proto.setPrototype(global.property("Object").property("prototype"));
    
    _ctor = engine->newFunction(construct, _proto);
    _ctor.setData(engine->toScriptValue(this));
    
    engine->globalObject().setProperty(name(), _ctor);
}

QScriptValue ArrayBufferClass::newInstance(unsigned long size) {
    engine()->reportAdditionalMemoryCost(size);
    QScriptValue data = engine()->newVariant(QVariant::fromValue(QByteArray(size, 0)));
    return engine()->newObject(this, data);
}

QScriptValue ArrayBufferClass::newInstance(const QByteArray& ba) {
    QScriptValue data = engine()->newVariant(QVariant::fromValue(ba));
    return engine()->newObject(this, data);
}

QScriptValue ArrayBufferClass::construct(QScriptContext* context, QScriptEngine* engine) {
    ArrayBufferClass* cls = qscriptvalue_cast<ArrayBufferClass*>(context->callee().data());
    if (!cls) {
        return QScriptValue();
    }
    
    QScriptValue arg = context->argument(0);
    unsigned long size = arg.toInt32();
    QScriptValue newObject = cls->newInstance(size);
    
    if (context->isCalledAsConstructor()) {
        context->setThisObject(newObject);
        return engine->undefinedValue();
    }
    
    return newObject;
}

QScriptClass::QueryFlags ArrayBufferClass::queryProperty(const QScriptValue& object,
                                                    const QScriptString& name,
                                                    QueryFlags flags, uint* id) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data());
    if (ba && name == _byteLength) {
        // if the property queried is byteLength, only handle read access
        return flags &= HandlesReadAccess;
    }
    return 0; // No access
}

QScriptValue ArrayBufferClass::property(const QScriptValue &object,
                                   const QScriptString &name, uint id) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data());
    if (ba && name == _byteLength) {
        return ba->length();
    }
    return QScriptValue();
}

QScriptValue::PropertyFlags ArrayBufferClass::propertyFlags(const QScriptValue& object,
                                                       const QScriptString& name, uint id) {
    return QScriptValue::Undeletable;
}

QString ArrayBufferClass::name() const {
    return _name.toString();
}

QScriptValue ArrayBufferClass::prototype() const {
    return _proto;
}

QScriptValue ArrayBufferClass::toScriptValue(QScriptEngine* engine, const QByteArray& ba) {
    QScriptValue ctor = engine->globalObject().property(CLASS_NAME);
    ArrayBufferClass* cls = qscriptvalue_cast<ArrayBufferClass*>(ctor.data());
    if (!cls) {
        return engine->newVariant(QVariant::fromValue(ba));
    }
    return cls->newInstance(ba);
}

void ArrayBufferClass::fromScriptValue(const QScriptValue& obj, QByteArray& ba) {
    ba = qvariant_cast<QByteArray>(obj.data().toVariant());
}

