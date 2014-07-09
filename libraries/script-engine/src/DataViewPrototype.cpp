//
//  DataViewPrototype.cpp
//
//
//  Created by Clement on 7/8/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <QDebug>

#include <glm/glm.hpp>

#include "DataViewClass.h"

#include "DataViewPrototype.h"

Q_DECLARE_METATYPE(QByteArray*)

DataViewPrototype::DataViewPrototype(QObject* parent) : QObject(parent) {
}

QByteArray* DataViewPrototype::thisArrayBuffer() const {
    QScriptValue bufferObject = thisObject().data().property(BUFFER_PROPERTY_NAME);
    return qscriptvalue_cast<QByteArray*>(bufferObject.data());
}

bool DataViewPrototype::realOffset(quint32& offset, size_t size) const {
    quint32 viewOffset = thisObject().data().property(BYTE_OFFSET_PROPERTY_NAME).toInt32();
    quint32 viewLength = thisObject().data().property(BYTE_LENGTH_PROPERTY_NAME).toInt32();
    //qDebug() << "View Offset: " << viewOffset << ", View Lenght: " << viewLength;
    //qDebug() << "Offset: " << offset << ", Size: " << size;
    offset += viewOffset;
    //qDebug() << "New offset: " << offset << ", bool: " << ((offset + size) <= viewOffset + viewLength);
    return (offset + size) <= viewOffset + viewLength;
}

///////////////// GETTERS ////////////////////////////

qint32 DataViewPrototype::getInt8(quint32 byteOffset) {
    if (realOffset(byteOffset, sizeof(qint8))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        
        qint8 result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

quint32 DataViewPrototype::getUint8(quint32 byteOffset) {
    if (realOffset(byteOffset, sizeof(quint8))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        
        quint8 result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

qint32 DataViewPrototype::getInt16(quint32 byteOffset, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(qint16))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        qint16 result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

quint32 DataViewPrototype::getUint16(quint32 byteOffset, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(quint16))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        quint16 result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

qint32 DataViewPrototype::getInt32(quint32 byteOffset, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(qint32))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        qint32 result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

quint32 DataViewPrototype::getUint32(quint32 byteOffset, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(quint32))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        quint32 result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

float DataViewPrototype::getFloat32(quint32 byteOffset, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(float))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        float result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

double DataViewPrototype::getFloat64(quint32 byteOffset, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(double))) {
        QDataStream stream(*thisArrayBuffer());
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        double result;
        stream >> result;
        return result;
    }
    thisObject().engine()->evaluate("throw \"RangeError: byteOffset out of range\"");
    return 0;
}

///////////////// SETTERS ////////////////////////////

void DataViewPrototype::setInt8(quint32 byteOffset, qint32 value) {
    if (realOffset(byteOffset, sizeof(qint8))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        
        stream << (qint8)value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setUint8(quint32 byteOffset, quint32 value) {
    if (realOffset(byteOffset, sizeof(quint8))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        
        stream << (quint8)value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setInt16(quint32 byteOffset, qint32 value, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(qint16))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        stream << (qint16)value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setUint16(quint32 byteOffset, quint32 value, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(quint16))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        stream << (quint16)value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setInt32(quint32 byteOffset, qint32 value, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(qint32))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        stream << (qint32)value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setUint32(quint32 byteOffset, quint32 value, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(quint32))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        stream << (quint32)value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setFloat32(quint32 byteOffset, float value, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(float))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        stream << value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}

void DataViewPrototype::setFloat64(quint32 byteOffset, double value, bool littleEndian) {
    if (realOffset(byteOffset, sizeof(double))) {
        QDataStream stream(thisArrayBuffer(), QIODevice::ReadWrite);
        stream.skipRawData(byteOffset);
        stream.setByteOrder((littleEndian) ? QDataStream::LittleEndian : QDataStream::BigEndian);
        
        stream << value;
    } else {
        thisObject().engine()->evaluate("throw \"1RangeError: byteOffset out of range\"");
    }
}


