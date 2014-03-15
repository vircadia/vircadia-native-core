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
#include <QMetaType>
#include <QUrl>
#include <QtDebug>

#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>

#include "AttributeRegistry.h"
#include "Bitstream.h"
#include "ScriptCache.h"

REGISTER_SIMPLE_TYPE_STREAMER(bool)
REGISTER_SIMPLE_TYPE_STREAMER(int)
REGISTER_SIMPLE_TYPE_STREAMER(uint)
REGISTER_SIMPLE_TYPE_STREAMER(float)
REGISTER_SIMPLE_TYPE_STREAMER(QByteArray)
REGISTER_SIMPLE_TYPE_STREAMER(QColor)
REGISTER_SIMPLE_TYPE_STREAMER(QString)
REGISTER_SIMPLE_TYPE_STREAMER(QUrl)
REGISTER_SIMPLE_TYPE_STREAMER(QVariantList)
REGISTER_SIMPLE_TYPE_STREAMER(QVariantHash)

// some types don't quite work with our macro
static int vec3Streamer = Bitstream::registerTypeStreamer(qMetaTypeId<glm::vec3>(), new SimpleTypeStreamer<glm::vec3>());
static int metaObjectStreamer = Bitstream::registerTypeStreamer(qMetaTypeId<const QMetaObject*>(),
    new SimpleTypeStreamer<const QMetaObject*>());

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
    return *this;
}

IDStreamer& IDStreamer::operator>>(int& value) {
    value = 0;
    _stream.read(&value, _bits);
    if (value == (1 << _bits) - 1) {
        _bits++;
    }
    return *this;
}

int Bitstream::registerMetaObject(const char* className, const QMetaObject* metaObject) {
    getMetaObjects().insert(className, metaObject);
    
    // register it as a subclass of itself and all of its superclasses
    for (const QMetaObject* superClass = metaObject; superClass; superClass = superClass->superClass()) {
        getMetaObjectSubClasses().insert(superClass, metaObject);
    }
    return 0;
}

int Bitstream::registerTypeStreamer(int type, TypeStreamer* streamer) {
    streamer->setType(type);
    getTypeStreamers().insert(type, streamer);
    return 0;
}

const QMetaObject* Bitstream::getMetaObject(const QByteArray& className) {
    return getMetaObjects().value(className);
}

QList<const QMetaObject*> Bitstream::getMetaObjectSubClasses(const QMetaObject* metaObject) {
    return getMetaObjectSubClasses().values(metaObject);
}

Bitstream::Bitstream(QDataStream& underlying, MetadataType metadataType, QObject* parent) :
    QObject(parent),
    _underlying(underlying),
    _byte(0),
    _position(0),
    _metadataType(metadataType),
    _metaObjectStreamer(*this),
    _typeStreamerStreamer(*this),
    _attributeStreamer(*this),
    _scriptStringStreamer(*this),
    _sharedObjectStreamer(*this) {
}

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
        int mask = ((1 << bitsToRead) - 1) << offset;
        *dest = (*dest & ~mask) | (((_byte >> _position) << offset) & mask);
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
        _typeStreamerStreamer.getAndResetTransientOffsets(),
        _attributeStreamer.getAndResetTransientOffsets(),
        _scriptStringStreamer.getAndResetTransientOffsets(),
        _sharedObjectStreamer.getAndResetTransientOffsets() };
    return mappings;
}

void Bitstream::persistWriteMappings(const WriteMappings& mappings) {
    _metaObjectStreamer.persistTransientOffsets(mappings.metaObjectOffsets);
    _typeStreamerStreamer.persistTransientOffsets(mappings.typeStreamerOffsets);
    _attributeStreamer.persistTransientOffsets(mappings.attributeOffsets);
    _scriptStringStreamer.persistTransientOffsets(mappings.scriptStringOffsets);
    _sharedObjectStreamer.persistTransientOffsets(mappings.sharedObjectOffsets);
    
    // find out when shared objects are deleted in order to clear their mappings
    for (QHash<SharedObjectPointer, int>::const_iterator it = mappings.sharedObjectOffsets.constBegin();
            it != mappings.sharedObjectOffsets.constEnd(); it++) {
        if (it.key()) {
            connect(it.key().data(), SIGNAL(destroyed(QObject*)), SLOT(clearSharedObject(QObject*)));
        }
    }
}

void Bitstream::persistAndResetWriteMappings() {
    persistWriteMappings(getAndResetWriteMappings());
}

Bitstream::ReadMappings Bitstream::getAndResetReadMappings() {
    ReadMappings mappings = { _metaObjectStreamer.getAndResetTransientValues(),
        _typeStreamerStreamer.getAndResetTransientValues(),
        _attributeStreamer.getAndResetTransientValues(),
        _scriptStringStreamer.getAndResetTransientValues(),
        _sharedObjectStreamer.getAndResetTransientValues() };
    return mappings;
}

void Bitstream::persistReadMappings(const ReadMappings& mappings) {
    _metaObjectStreamer.persistTransientValues(mappings.metaObjectValues);
    _typeStreamerStreamer.persistTransientValues(mappings.typeStreamerValues);
    _attributeStreamer.persistTransientValues(mappings.attributeValues);
    _scriptStringStreamer.persistTransientValues(mappings.scriptStringValues);
    _sharedObjectStreamer.persistTransientValues(mappings.sharedObjectValues);
}

void Bitstream::persistAndResetReadMappings() {
    persistReadMappings(getAndResetReadMappings());
}

void Bitstream::clearSharedObject(int id) {
    SharedObjectPointer object = _sharedObjectStreamer.takePersistentValue(id);
    if (object) {
        _weakSharedObjectHash.remove(object->getRemoteID());
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

Bitstream& Bitstream::operator<<(int value) {
    return write(&value, 32);
}

Bitstream& Bitstream::operator>>(int& value) {
    qint32 sizedValue;
    read(&sizedValue, 32);
    value = sizedValue;
    return *this;
}

Bitstream& Bitstream::operator<<(uint value) {
    return write(&value, 32);
}

Bitstream& Bitstream::operator>>(uint& value) {
    quint32 sizedValue;
    read(&sizedValue, 32);
    value = sizedValue;
    return *this;
}

Bitstream& Bitstream::operator<<(float value) {
    return write(&value, 32);
}

Bitstream& Bitstream::operator>>(float& value) {
    return read(&value, 32);
}

Bitstream& Bitstream::operator<<(const glm::vec3& value) {
    return *this << value.x << value.y << value.z;
}

Bitstream& Bitstream::operator>>(glm::vec3& value) {
    return *this >> value.x >> value.y >> value.z;
}

Bitstream& Bitstream::operator<<(const QByteArray& string) {
    *this << string.size();
    return write(string.constData(), string.size() * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator>>(QByteArray& string) {
    int size;
    *this >> size;
    string.resize(size);
    return read(string.data(), size * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator<<(const QColor& color) {
    return *this << (int)color.rgba();
}

Bitstream& Bitstream::operator>>(QColor& color) {
    int rgba;
    *this >> rgba;
    color.setRgba(rgba);
    return *this;
}

Bitstream& Bitstream::operator<<(const QString& string) {
    *this << string.size();    
    return write(string.constData(), string.size() * sizeof(QChar) * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator>>(QString& string) {
    int size;
    *this >> size;
    string.resize(size);
    return read(string.data(), size * sizeof(QChar) * BITS_IN_BYTE);
}

Bitstream& Bitstream::operator<<(const QUrl& url) {
    return *this << url.toString();
}

Bitstream& Bitstream::operator>>(QUrl& url) {
    QString string;
    *this >> string;
    url = string;
    return *this;
}

Bitstream& Bitstream::operator<<(const QVariant& value) {
    const TypeStreamer* streamer = getTypeStreamers().value(value.userType());
    if (streamer) {
        _typeStreamerStreamer << streamer;
        streamer->write(*this, value);
    } else {
        qWarning() << "Non-streamable type: " << value.typeName() << "\n";
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QVariant& value) {
    const TypeStreamer* streamer;
    _typeStreamerStreamer >> streamer;
    if (streamer) {
        value = streamer->read(*this);
    }
    return *this;
}

Bitstream& Bitstream::operator<<(const AttributeValue& attributeValue) {
    _attributeStreamer << attributeValue.getAttribute();
    if (attributeValue.getAttribute()) {
        attributeValue.getAttribute()->write(*this, attributeValue.getValue(), true);
    }
    return *this;
}

Bitstream& Bitstream::operator>>(OwnedAttributeValue& attributeValue) {
    AttributePointer attribute;
    _attributeStreamer >> attribute;
    if (attribute) {
        void* value = attribute->create();
        attribute->read(*this, value, true);
        attributeValue = AttributeValue(attribute, value);
        attribute->destroy(value);
        
    } else {
        attributeValue = AttributeValue();
    }
    return *this;
}

Bitstream& Bitstream::operator<<(const QObject* object) {
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
        const TypeStreamer* streamer = getTypeStreamers().value(property.userType());
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
    readProperties(object = metaObject->newInstance());
    return *this;
}

Bitstream& Bitstream::operator<<(const QMetaObject* metaObject) {
    _metaObjectStreamer << metaObject;
    return *this;
}

Bitstream& Bitstream::operator>>(const QMetaObject*& metaObject) {
    _metaObjectStreamer >> metaObject;
    return *this;
}

Bitstream& Bitstream::operator<<(const TypeStreamer* streamer) {
    _typeStreamerStreamer << streamer;    
    return *this;
}

Bitstream& Bitstream::operator>>(const TypeStreamer*& streamer) {
    _typeStreamerStreamer >> streamer;
    return *this;
}

Bitstream& Bitstream::operator<<(const AttributePointer& attribute) {
    _attributeStreamer << attribute;
    return *this;
}

Bitstream& Bitstream::operator>>(AttributePointer& attribute) {
    _attributeStreamer >> attribute;
    return *this;
}

Bitstream& Bitstream::operator<<(const QScriptString& string) {
    _scriptStringStreamer << string;
    return *this;
}

Bitstream& Bitstream::operator>>(QScriptString& string) {
    _scriptStringStreamer >> string;
    return *this;
}

Bitstream& Bitstream::operator<<(const SharedObjectPointer& object) {
    _sharedObjectStreamer << object;
    return *this;
}

Bitstream& Bitstream::operator>>(SharedObjectPointer& object) {
    _sharedObjectStreamer >> object;
    return *this;
}

Bitstream& Bitstream::operator<(const QMetaObject* metaObject) {
    return *this << (metaObject ? QByteArray::fromRawData(metaObject->className(),
        strlen(metaObject->className())) : QByteArray());
}

Bitstream& Bitstream::operator>(const QMetaObject*& metaObject) {
    QByteArray className;
    *this >> className;
    if (className.isEmpty()) {
        metaObject = NULL;
        return *this;
    }
    metaObject = getMetaObjects().value(className);
    if (!metaObject) {
        qWarning() << "Unknown class name: " << className << "\n";
    }
    return *this;
}

Bitstream& Bitstream::operator<(const TypeStreamer* streamer) {
    const char* typeName = QMetaType::typeName(streamer->getType());
    return *this << QByteArray::fromRawData(typeName, strlen(typeName));
}

Bitstream& Bitstream::operator>(const TypeStreamer*& streamer) {
    QByteArray typeName;
    *this >> typeName;
    streamer = getTypeStreamers().value(QMetaType::type(typeName.constData()));
    if (!streamer) {
        qWarning() << "Unknown type name: " << typeName << "\n";
    }
    return *this;
}

Bitstream& Bitstream::operator<(const AttributePointer& attribute) {
    return *this << (QObject*)attribute.data();
}

Bitstream& Bitstream::operator>(AttributePointer& attribute) {
    QObject* object;
    *this >> object;
    attribute = AttributeRegistry::getInstance()->registerAttribute(static_cast<Attribute*>(object));
    return *this;
}

Bitstream& Bitstream::operator<(const QScriptString& string) {
    return *this << string.toString();
}

Bitstream& Bitstream::operator>(QScriptString& string) {
    QString rawString;
    *this >> rawString;
    string = ScriptCache::getInstance()->getEngine()->toStringHandle(rawString);
    return *this;
}

Bitstream& Bitstream::operator<(const SharedObjectPointer& object) {
    if (!object) {
        return *this << (int)0;
    }
    return *this << object->getID() << (QObject*)object.data();
}

Bitstream& Bitstream::operator>(SharedObjectPointer& object) {
    int id;
    *this >> id;
    if (id == 0) {
        object = SharedObjectPointer();
        return *this;
    }
    QPointer<SharedObject>& pointer = _weakSharedObjectHash[id];
    if (pointer) {
        const QMetaObject* metaObject;
        _metaObjectStreamer >> metaObject;
        if (metaObject != pointer->metaObject()) {
            qWarning() << "Class mismatch: " << pointer->metaObject()->className() << metaObject->className();
        }
        readProperties(pointer.data());
    
    } else {
        QObject* rawObject;
        *this >> rawObject;
        pointer = static_cast<SharedObject*>(rawObject);
        pointer->setRemoteID(id);
    }
    object = static_cast<SharedObject*>(pointer.data());
    return *this;
}

void Bitstream::clearSharedObject(QObject* object) {
    int id = _sharedObjectStreamer.takePersistentID(static_cast<SharedObject*>(object));
    if (id != 0) {
        emit sharedObjectCleared(id);
    }
}

void Bitstream::readProperties(QObject* object) {
    const QMetaObject* metaObject = object->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (!property.isStored(object)) {
            continue;
        }
        const TypeStreamer* streamer = getTypeStreamers().value(property.userType());
        if (streamer) {
            property.write(object, streamer->read(*this));    
        }
    }
}

QHash<QByteArray, const QMetaObject*>& Bitstream::getMetaObjects() {
    static QHash<QByteArray, const QMetaObject*> metaObjects;
    return metaObjects;
}

QMultiHash<const QMetaObject*, const QMetaObject*>& Bitstream::getMetaObjectSubClasses() {
    static QMultiHash<const QMetaObject*, const QMetaObject*> metaObjectSubClasses;
    return metaObjectSubClasses;
}

QHash<int, const TypeStreamer*>& Bitstream::getTypeStreamers() {
    static QHash<int, const TypeStreamer*> typeStreamers;
    return typeStreamers;
}

