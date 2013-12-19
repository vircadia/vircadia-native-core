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

#include "Bitstream.h"

QHash<QByteArray, const QMetaObject*> Bitstream::_metaObjects;
QHash<int, TypeStreamer*> Bitstream::_typeStreamers;

void Bitstream::registerMetaObject(const QByteArray& name, const QMetaObject* metaObject) {
    _metaObjects.insert(name, metaObject);
}

void Bitstream::registerTypeStreamer(int type, TypeStreamer* streamer) {
    _typeStreamers.insert(type, streamer);
}

Bitstream::Bitstream(QDataStream& underlying) :
    _underlying(underlying), _byte(0), _position(0), _classNameStreamer(*this) {
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
        _byte = 0;
        _position = 0;
    }
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

Bitstream& Bitstream::operator<<(qint32 value) {
    return write(&value, 32);
}

Bitstream& Bitstream::operator>>(qint32& value) {
    return read(&value, 32);
}

Bitstream& Bitstream::operator<<(const QByteArray& string) {
    *this << (qint32)string.size();
    return write(string.constData(), string.size() * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator>>(QByteArray& string) {
    qint32 size;
    *this >> size;
    return read(string.data(), size * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator<<(const QString& string) {
    *this << (qint32)string.size();    
    return write(string.constData(), string.size() * sizeof(QChar) * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator>>(QString& string) {
    qint32 size;
    *this >> size;
    return read(string.data(), size * sizeof(QChar) * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator<<(QObject* object) {
    const QMetaObject* metaObject = object->metaObject();
    _classNameStreamer << QByteArray::fromRawData(metaObject->className(), strlen(metaObject->className()));
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (!property.isStored(object)) {
            continue;
        }
        TypeStreamer* streamer = _typeStreamers.value(property.userType());
        if (streamer) {
            streamer->write(*this, property.read(object));
        }
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QObject*& object) {
    QByteArray className;
    _classNameStreamer >> className;
    const QMetaObject* metaObject = _metaObjects.value(className);
    if (metaObject) {
        object = metaObject->newInstance();
        for (int i = 0; i < metaObject->propertyCount(); i++) {
            QMetaProperty property = metaObject->property(i);
            if (!property.isStored(object)) {
                continue;
            }
            TypeStreamer* streamer = _typeStreamers.value(property.userType());
            if (streamer) {
                property.write(object, streamer->read(*this));    
            }
        }
    } else {
        qDebug() << "Unknown class name: " << className << "\n";
    }
    return *this;
}

IDStreamer::IDStreamer(Bitstream& stream) :
    _stream(stream),
    _bits(1) {
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
