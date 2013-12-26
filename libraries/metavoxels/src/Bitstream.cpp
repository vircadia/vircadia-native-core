//
//  Bitstream.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>

#include <QDataStream>
#include <QMetaProperty>
#include <QtDebug>

#include "AttributeRegistry.h"
#include "Bitstream.h"

REGISTER_SIMPLE_TYPE_STREAMER(QByteArray)
REGISTER_SIMPLE_TYPE_STREAMER(QString)
REGISTER_SIMPLE_TYPE_STREAMER(bool)
REGISTER_SIMPLE_TYPE_STREAMER(int)

IDStreamer::IDStreamer(Bitstream& stream) :
    _stream(stream),
    _bits(1) {
}

void IDStreamer::setBitsFromValue(int value) {
    _bits = 1;
    while (value >= (1 << _bits) - 1) {
        _bits++;
    }
}

IDStreamer& IDStreamer::operator<<(int value) {
    _stream.write(&value, _bits);
    if (value == (1 << _bits) - 1) {
        _bits++;
    }
}

IDStreamer& IDStreamer::operator>>(int& value) {
    _stream.read(&value, _bits);
    if (value == (1 << _bits) - 1) {
        _bits++;
    }
}

int Bitstream::registerMetaObject(const char* className, const QMetaObject* metaObject) {
    getMetaObjects().insert(className, metaObject);
    return 0;
}

int Bitstream::registerTypeStreamer(int type, TypeStreamer* streamer) {
    getTypeStreamers().insert(type, streamer);
    return 0;
}

Bitstream::Bitstream(QDataStream& underlying) :
    _underlying(underlying),
    _byte(0),
    _position(0),
    _metaObjectStreamer(*this),
    _attributeStreamer(*this) {
}

const int BITS_IN_BYTE = 8;
const int LAST_BIT_POSITION = BITS_IN_BYTE - 1;

Bitstream& Bitstream::write(const void* data, int bits, int offset) {
    const quint8* source = (const quint8*)data;
    while (bits > 0) {
        int bitsToWrite = qMin(BITS_IN_BYTE - _position, qMin(BITS_IN_BYTE - offset, bits));
        _byte |= ((*source >> offset) & ((1 << bitsToWrite) - 1)) << _position;
        if ((_position += bitsToWrite) == BITS_IN_BYTE) {
            flush();
        }
        if ((offset += bitsToWrite) == BITS_IN_BYTE) {
            source++;
            offset = 0;
        }
        bits -= bitsToWrite;
    }
    return *this;
}

Bitstream& Bitstream::read(void* data, int bits, int offset) {
    quint8* dest = (quint8*)data;
    while (bits > 0) {
        if (_position == 0) {
            _underlying >> _byte;
        }
        int bitsToRead = qMin(BITS_IN_BYTE - _position, qMin(BITS_IN_BYTE - offset, bits));
        *dest |= ((_byte >> _position) & ((1 << bitsToRead) - 1)) << offset;
        _position = (_position + bitsToRead) & LAST_BIT_POSITION;
        if ((offset += bitsToRead) == BITS_IN_BYTE) {
            dest++;
            offset = 0;
        }
        bits -= bitsToRead;
    }
    return *this;
}

void Bitstream::flush() {
    if (_position != 0) {
        _underlying << _byte;
        reset();
    }
}

void Bitstream::reset() {
    _byte = 0;
    _position = 0;
}

Bitstream::WriteMappings Bitstream::getAndResetWriteMappings() {
    WriteMappings mappings = { _metaObjectStreamer.getAndResetTransientOffsets(),
        _attributeStreamer.getAndResetTransientOffsets() };
    return mappings;
}

void Bitstream::persistWriteMappings(const WriteMappings& mappings) {
    _metaObjectStreamer.persistTransientOffsets(mappings.metaObjectOffsets);
    _attributeStreamer.persistTransientOffsets(mappings.attributeOffsets);
}

Bitstream::ReadMappings Bitstream::getAndResetReadMappings() {
    ReadMappings mappings = { _metaObjectStreamer.getAndResetTransientValues(),
        _attributeStreamer.getAndResetTransientValues() };
    return mappings;
}

void Bitstream::persistReadMappings(const ReadMappings& mappings) {
    _metaObjectStreamer.persistTransientValues(mappings.metaObjectValues);
    _attributeStreamer.persistTransientValues(mappings.attributeValues);
}

Bitstream& Bitstream::operator<<(bool value) {
    if (value) {
        _byte |= (1 << _position);
    }
    if (++_position == BITS_IN_BYTE) {
        flush();
    }
    return *this;
}

Bitstream& Bitstream::operator>>(bool& value) {
    if (_position == 0) {
        _underlying >> _byte;
    }
    value = _byte & (1 << _position);
    _position = (_position + 1) & LAST_BIT_POSITION;
    return *this;
}

Bitstream& Bitstream::operator<<(int value) {
    return write(&value, 32);
}

Bitstream& Bitstream::operator>>(int& value) {
    qint32 sizedValue;
    read(&sizedValue, 32);
    value = sizedValue;
    return *this;
}

Bitstream& Bitstream::operator<<(const QByteArray& string) {
    *this << string.size();
    return write(string.constData(), string.size() * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator>>(QByteArray& string) {
    int size;
    *this >> size;
    return read(string.data(), size * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator<<(const QString& string) {
    *this << string.size();    
    return write(string.constData(), string.size() * sizeof(QChar) * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator>>(QString& string) {
    int size;
    *this >> size;
    return read(string.data(), size * sizeof(QChar) * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator<<(const QVariant& value) {
    *this << value.userType();
    TypeStreamer* streamer = getTypeStreamers().value(value.userType());
    if (streamer) {
        streamer->write(*this, value);
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QVariant& value) {
    int type;
    *this >> type;
    TypeStreamer* streamer = getTypeStreamers().value(type);
    if (streamer) {
        value = streamer->read(*this);
    }
    return *this;
}

Bitstream& Bitstream::operator<<(QObject* object) {
    if (!object) {
        _metaObjectStreamer << NULL;
        return *this;
    }
    const QMetaObject* metaObject = object->metaObject();
    _metaObjectStreamer << metaObject;
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (!property.isStored(object)) {
            continue;
        }
        TypeStreamer* streamer = getTypeStreamers().value(property.userType());
        if (streamer) {
            streamer->write(*this, property.read(object));
        }
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QObject*& object) {
    const QMetaObject* metaObject;
    _metaObjectStreamer >> metaObject;
    if (!metaObject) {
        object = NULL;
        return *this;
    }
    object = metaObject->newInstance();
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (!property.isStored(object)) {
            continue;
        }
        TypeStreamer* streamer = getTypeStreamers().value(property.userType());
        if (streamer) {
            property.write(object, streamer->read(*this));    
        }
    }
    return *this;
}

Bitstream& Bitstream::operator<<(const QMetaObject* metaObject) {
    return *this << (metaObject ? QByteArray::fromRawData(
        metaObject->className(), strlen(metaObject->className())) : QByteArray());
}

Bitstream& Bitstream::operator>>(const QMetaObject*& metaObject) {
    QByteArray className;
    *this >> className;
    if (className.isEmpty()) {
        metaObject = NULL;
        return *this;
    }
    metaObject = getMetaObjects().value(className);
    if (!metaObject) {
        qDebug() << "Unknown class name: " << className << "\n";
    }
    return *this;
}

Bitstream& Bitstream::operator<<(const AttributePointer& attribute) {
    return *this << (QObject*)attribute.data();
}

Bitstream& Bitstream::operator>>(AttributePointer& attribute) {
    QObject* object;
    *this >> object;
    attribute = AttributeRegistry::getInstance()->registerAttribute(static_cast<Attribute*>(object));
    return *this;
}

QHash<QByteArray, const QMetaObject*>& Bitstream::getMetaObjects() {
    static QHash<QByteArray, const QMetaObject*> metaObjects;
    return metaObjects;
}

QHash<int, TypeStreamer*>& Bitstream::getTypeStreamers() {
    static QHash<int, TypeStreamer*> typeStreamers;
    return typeStreamers;
}

