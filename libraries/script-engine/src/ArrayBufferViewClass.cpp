//
//  ArrayBufferViewClass.cpp
//
//
//  Created by Clement on 7/8/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ArrayBufferViewClass.h"

Q_DECLARE_METATYPE(QByteArray*)

ArrayBufferViewClass::ArrayBufferViewClass(QScriptEngine* engine) :
QObject(engine),
QScriptClass(engine) {
    // Save string handles for quick lookup
    _bufferName = engine->toStringHandle(BUFFER_PROPERTY_NAME.toLatin1());
    _byteOffsetName = engine->toStringHandle(BYTE_OFFSET_PROPERTY_NAME.toLatin1());
    _byteLengthName = engine->toStringHandle(BYTE_LENGTH_PROPERTY_NAME.toLatin1());
}

QScriptClass::QueryFlags ArrayBufferViewClass::queryProperty(const QScriptValue& object,
                                                             const QScriptString& name,
                                                             QueryFlags flags, uint* id) {
    if (name == _bufferName || name == _byteOffsetName || name == _byteLengthName) {
        return flags &= HandlesReadAccess; // Only keep read access flags
    }
    return 0; // No access
}

QScriptValue ArrayBufferViewClass::property(const QScriptValue &object,
                                            const QScriptString &name, uint id) {
    if (name == _bufferName) {
        return object.data().property(_bufferName);
    }
    if (name == _byteOffsetName) {
        return object.data().property(_byteOffsetName);
    }
    if (name == _byteLengthName) {
        return object.data().property(_byteLengthName);
    }
    return QScriptValue();
}

QScriptValue::PropertyFlags ArrayBufferViewClass::propertyFlags(const QScriptValue& object,
                                                                const QScriptString& name, uint id) {
    return QScriptValue::Undeletable;
}

//QScriptClass::QueryFlags DataViewClass::queryProperty(const QScriptValue& object,
//                                                 const QScriptString& name,
//                                                      QueryFlags flags, uint* id) {
//    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.property(_bufferName).data());
//    bool ok = false;
//    int pos = name.toArrayIndex(&ok);
//
//    // Check that name is a valid index and arrayBuffer exists
//    if (ok && arrayBuffer && pos > 0 && pos < arrayBuffer->size()) {
//        *id = pos; // save pos to avoid recomputation
//        return HandlesReadAccess | HandlesWriteAccess; // Read/Write access
//    }
//
//    return ArrayBufferViewClass::queryProperty(object, name, flags, id);
//}
//
//QScriptValue DataViewClass::property(const QScriptValue &object,
//                                const QScriptString &name, uint id) {
//    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.property(_bufferName).data());
//    bool ok = false;
//    name.toArrayIndex(&ok);
//
//    if (ok && arrayBuffer) {
//        return (*arrayBuffer)[id];
//    }
//
//    return ArrayBufferViewClass::queryProperty(object, name, flags, id);
//}
//
//void DataViewClass::setProperty(QScriptValue &object,
//                                const QScriptString &name,
//                                uint id, const QScriptValue &value) {
//    QByteArray *ba = qscriptvalue_cast<QByteArray*>(object.data());
//    if (!ba)
//    return;
//    if (name == length) {
//        resize(*ba, value.toInt32());
//    } else {
//        qint32 pos = id;
//        if (pos < 0)
//        return;
//        if (ba->size() <= pos)
//        resize(*ba, pos + 1);
//        (*ba)[pos] = char(value.toInt32());
//    }
//}
//
//QScriptValue::PropertyFlags DataViewClass::propertyFlags(const QScriptValue& object,
//                                                    const QScriptString& name, uint id) {
//    return QScriptValue::Undeletable;
//}
