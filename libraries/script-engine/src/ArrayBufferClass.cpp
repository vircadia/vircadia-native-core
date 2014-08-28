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

#include <QDebug>

#include "ArrayBufferPrototype.h"
#include "DataViewClass.h"
#include "ScriptEngine.h"
#include "TypedArrays.h"

#include "ArrayBufferClass.h"

static const QString CLASS_NAME = "ArrayBuffer";

Q_DECLARE_METATYPE(QByteArray*)

ArrayBufferClass::ArrayBufferClass(ScriptEngine* scriptEngine) :
QObject(scriptEngine),
QScriptClass(scriptEngine),
_scriptEngine(scriptEngine) {
    qScriptRegisterMetaType<QByteArray>(engine(), toScriptValue, fromScriptValue);
    QScriptValue global = engine()->globalObject();
    
    // Save string handles for quick lookup
    _name = engine()->toStringHandle(CLASS_NAME.toLatin1());
    _byteLength = engine()->toStringHandle(BYTE_LENGTH_PROPERTY_NAME.toLatin1());
    
    // build prototype
    _proto = engine()->newQObject(new ArrayBufferPrototype(this),
                                QScriptEngine::QtOwnership,
                                QScriptEngine::SkipMethodsInEnumeration |
                                QScriptEngine::ExcludeSuperClassMethods |
                                QScriptEngine::ExcludeSuperClassProperties);
    _proto.setPrototype(global.property("Object").property("prototype"));
    
    // Register constructor
    _ctor = engine()->newFunction(construct, _proto);
    _ctor.setData(engine()->toScriptValue(this));
    
    engine()->globalObject().setProperty(name(), _ctor);
    
    // Registering other array types
    // The script engine is there parent so it'll delete them with itself
    new DataViewClass(scriptEngine);
    new Int8ArrayClass(scriptEngine);
    new Uint8ArrayClass(scriptEngine);
    new Uint8ClampedArrayClass(scriptEngine);
    new Int16ArrayClass(scriptEngine);
    new Uint16ArrayClass(scriptEngine);
    new Int32ArrayClass(scriptEngine);
    new Uint32ArrayClass(scriptEngine);
    new Float32ArrayClass(scriptEngine);
    new Float64ArrayClass(scriptEngine);
}

QScriptValue ArrayBufferClass::newInstance(qint32 size) {
    const qint32 MAX_LENGTH = 100000000;
    if (size < 0) {
        engine()->evaluate("throw \"ArgumentError: negative length\"");
        return QScriptValue();
    }
    if (size > MAX_LENGTH) {
        engine()->evaluate("throw \"ArgumentError: absurd length\"");
        return QScriptValue();
    }
    
    engine()->reportAdditionalMemoryCost(size);
    QScriptEngine* eng = engine();
    QVariant variant = QVariant::fromValue(QByteArray(size, 0));
    QScriptValue data =  eng->newVariant(variant);
    return engine()->newObject(this, data);
}

QScriptValue ArrayBufferClass::newInstance(const QByteArray& ba) {
    QScriptValue data = engine()->newVariant(QVariant::fromValue(ba));
    return engine()->newObject(this, data);
}

QScriptValue ArrayBufferClass::construct(QScriptContext* context, QScriptEngine* engine) {
    ArrayBufferClass* cls = qscriptvalue_cast<ArrayBufferClass*>(context->callee().data());
    if (!cls) {
        // return if callee (function called) is not of type ArrayBuffer
        return QScriptValue();
    }
    QScriptValue arg = context->argument(0);
    if (!arg.isValid() || !arg.isNumber()) {
        return QScriptValue();
    }
    
    quint32 size = arg.toInt32();
    QScriptValue newObject = cls->newInstance(size);
    
    if (context->isCalledAsConstructor()) {
        // if called with keyword new, replace this object.
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

QScriptValue ArrayBufferClass::property(const QScriptValue& object,
                                   const QScriptString& name, uint id) {
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

