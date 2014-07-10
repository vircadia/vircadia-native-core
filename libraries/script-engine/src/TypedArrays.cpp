//
//  TypedArrays.cpp
//
//
//  Created by Clement on 7/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngine.h"

#include "TypedArrays.h"

Q_DECLARE_METATYPE(QByteArray*)

TypedArray::TypedArray(ScriptEngine* scriptEngine, QString name) : ArrayBufferViewClass(scriptEngine) {
    _bytesPerElementName = engine()->toStringHandle(BYTES_PER_ELEMENT_PROPERTY_NAME.toLatin1());
    _lengthName = engine()->toStringHandle(LENGTH_PROPERTY_NAME.toLatin1());
    _name = engine()->toStringHandle(name.toLatin1());
}

QScriptValue TypedArray::construct(QScriptContext *context, QScriptEngine *engine) {
    TypedArray* cls = qscriptvalue_cast<TypedArray*>(context->callee().data());
    if (!cls || context->argumentCount() < 1) {
        return QScriptValue();
    }
    
    QScriptValue newObject;
    QScriptValue bufferArg = context->argument(0);
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(bufferArg.data());
    TypedArray* typedArray = qscriptvalue_cast<TypedArray*>(bufferArg);
    
    if (arrayBuffer) {
        if (context->argumentCount() == 1) {
            // Case for entire ArrayBuffer
            newObject = cls->newInstance(bufferArg, 0, arrayBuffer->size());
        } else {
            QScriptValue byteOffsetArg = context->argument(1);
            if (!byteOffsetArg.isNumber()) {
                engine->evaluate("throw \"ArgumentError: 2nd arg is not a number\"");
                return QScriptValue();
            }
            if (byteOffsetArg.toInt32() < 0 || byteOffsetArg.toInt32() >= arrayBuffer->size()) {
                engine->evaluate("throw \"RangeError: byteOffset out of range\"");
                return QScriptValue();
            }
            quint32 byteOffset = byteOffsetArg.toInt32();
            
            if (context->argumentCount() == 2) {
                // case for end of ArrayBuffer
                newObject = cls->newInstance(bufferArg, byteOffset, arrayBuffer->size() - byteOffset);
            } else {
                
                QScriptValue lengthArg = (context->argumentCount() > 2) ? context->argument(2) : QScriptValue();
                if (!lengthArg.isNumber()) {
                    engine->evaluate("throw \"ArgumentError: 3nd arg is not a number\"");
                    return QScriptValue();
                }
                if (lengthArg.toInt32() < 0 ||
                    byteOffsetArg.toInt32() + lengthArg.toInt32() * cls->_bytesPerElement > arrayBuffer->size()) {
                    engine->evaluate("throw \"RangeError: byteLength out of range\"");
                    return QScriptValue();
                }
                quint32 length = lengthArg.toInt32() * cls->_bytesPerElement;
                
                // case for well-defined range
                newObject = cls->newInstance(bufferArg, byteOffset, length);
            }
        }
    } else if (typedArray) {
        // case for another reference to another TypedArray
        newObject = cls->newInstance(bufferArg, true);
    } else if (context->argument(0).isNumber()) {
        // case for new ArrayBuffer
        newObject = cls->newInstance(context->argument(0).toInt32());
    } else {
        // TODO: Standard array case
    }
    
    if (context->isCalledAsConstructor()) {
        context->setThisObject(newObject);
        return engine->undefinedValue();
    }
    
    return newObject;
}

QScriptClass::QueryFlags TypedArray::queryProperty(const QScriptValue& object,
                                                   const QScriptString& name,
                                                   QueryFlags flags, uint* id) {
    if (name == _bytesPerElementName || name == _lengthName) {
        return flags &= HandlesReadAccess; // Only keep read access flags
    }
    return ArrayBufferViewClass::queryProperty(object, name, flags, id);
}

QScriptValue TypedArray::property(const QScriptValue &object,
                                      const QScriptString &name, uint id) {
    if (name == _bytesPerElementName) {
        return QScriptValue(_bytesPerElement);
    }
    if (name == _lengthName) {
        return object.data().property(_lengthName);
    }
    return ArrayBufferViewClass::property(object, name, id);
}

QScriptValue::PropertyFlags TypedArray::propertyFlags(const QScriptValue& object,
                                                      const QScriptString& name, uint id) {
    return QScriptValue::Undeletable;
}

QString TypedArray::name() const {
    return _name.toString();
}

QScriptValue TypedArray::prototype() const {
    return _proto;
}

Int8ArrayClass::Int8ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, INT_8_ARRAY_CLASS_NAME) {
    QScriptValue global = engine()->globalObject();
    
    _bytesPerElement = sizeof(qint8);
    
    // build prototype
//    _proto = engine->newQObject(new DataViewPrototype(this),
//                                QScriptEngine::QtOwnership,
//                                QScriptEngine::SkipMethodsInEnumeration |
//                                QScriptEngine::ExcludeSuperClassMethods |
//                                QScriptEngine::ExcludeSuperClassProperties);
    
    _proto.setPrototype(global.property("Object").property("prototype"));
    
    _ctor = engine()->newFunction(construct, _proto);
    _ctor.setData(engine()->toScriptValue(this));
    
    engine()->globalObject().setProperty(name(), _ctor);
}

QScriptValue Int8ArrayClass::newInstance(quint32 length) {
    ArrayBufferClass* array = getScriptEngine()->getArrayBufferClass();
    QScriptValue buffer = array->newInstance(length * _bytesPerElement);
    return newInstance(buffer, 0, length);
}

QScriptValue Int8ArrayClass::newInstance(QScriptValue array, bool isTypedArray) {
    // TODO
    assert(false);
    if (isTypedArray) {
        QByteArray* originalArray = qscriptvalue_cast<QByteArray*>(array.data().property(_bufferName).data());
        ArrayBufferClass* arrayBufferClass = reinterpret_cast<ScriptEngine*>(engine())->getArrayBufferClass();
        quint32 byteOffset = array.data().property(_byteOffsetName).toInt32();
        quint32 byteLength = array.data().property(_byteLengthName).toInt32();
        quint32 length = byteLength / _bytesPerElement;
        QByteArray newArray = originalArray->mid(byteOffset, byteLength);
        
        if (byteLength % _bytesPerElement != 0) {
            length += 1;
            byteLength = length * _bytesPerElement;
        }
        
        
        return newInstance(arrayBufferClass->newInstance(newArray),
                           array.data().property(_byteOffsetName).toInt32(),
                           array.data().property(_lengthName).toInt32());
    }
    return QScriptValue();
}

QScriptValue Int8ArrayClass::newInstance(QScriptValue buffer, quint32 byteOffset, quint32 length) {
    QScriptValue data = engine()->newObject();
    data.setProperty(_bufferName, buffer);
    data.setProperty(_byteOffsetName, byteOffset);
    data.setProperty(_byteLengthName, length * _bytesPerElement);
    data.setProperty(_lengthName, length);
    
    return engine()->newObject(this, data);
}

QScriptClass::QueryFlags Int8ArrayClass::queryProperty(const QScriptValue& object,
                                                 const QScriptString& name,
                                                      QueryFlags flags, uint* id) {
    quint32 byteOffset = object.data().property(_byteOffsetName).toInt32();
    quint32 length = object.data().property(_lengthName).toInt32();
    bool ok = false;
    int pos = name.toArrayIndex(&ok);

    // Check that name is a valid index and arrayBuffer exists
    if (ok && pos >= 0 && pos < length) {
        *id = byteOffset + pos * _bytesPerElement; // save pos to avoid recomputation
        return HandlesReadAccess | HandlesWriteAccess; // Read/Write access
    }

    return TypedArray::queryProperty(object, name, flags, id);
}

QScriptValue Int8ArrayClass::property(const QScriptValue &object,
                                const QScriptString &name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    bool ok = false;
    name.toArrayIndex(&ok);

    if (ok && arrayBuffer) {
        QDataStream stream(*arrayBuffer);
        stream.skipRawData(id);
        
        qint8 result;
        stream >> result;
        return result;
    }

    return TypedArray::property(object, name, id);
}

void Int8ArrayClass::setProperty(QScriptValue &object,
                                const QScriptString &name,
                                uint id, const QScriptValue &value) {
    qDebug() << "ID: " << id << ", Value: " << value.toInt32();
    QByteArray *ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    if (ba && value.isNumber()) {
        QDataStream stream(ba, QIODevice::ReadWrite);
        stream.skipRawData(id);
        
        qDebug() << "Adding: " << (qint8)value.toInt32();
        stream << (qint8)value.toInt32();
    }
}



