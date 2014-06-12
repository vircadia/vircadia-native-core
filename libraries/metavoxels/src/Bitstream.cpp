//
//  Bitstream.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include <QCryptographicHash>
#include <QDataStream>
#include <QMetaType>
#include <QScriptValueIterator>
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
REGISTER_SIMPLE_TYPE_STREAMER(QScriptValue)
REGISTER_SIMPLE_TYPE_STREAMER(QString)
REGISTER_SIMPLE_TYPE_STREAMER(QUrl)
REGISTER_SIMPLE_TYPE_STREAMER(QVariantList)
REGISTER_SIMPLE_TYPE_STREAMER(QVariantHash)
REGISTER_SIMPLE_TYPE_STREAMER(SharedObjectPointer)

// some types don't quite work with our macro
static int vec3Streamer = Bitstream::registerTypeStreamer(qMetaTypeId<glm::vec3>(), new SimpleTypeStreamer<glm::vec3>());
static int quatStreamer = Bitstream::registerTypeStreamer(qMetaTypeId<glm::quat>(), new SimpleTypeStreamer<glm::quat>());
static int metaObjectStreamer = Bitstream::registerTypeStreamer(qMetaTypeId<const QMetaObject*>(),
    new SimpleTypeStreamer<const QMetaObject*>());

static int genericValueStreamer = Bitstream::registerTypeStreamer(
    qRegisterMetaType<GenericValue>(), new GenericValueStreamer());

static int qVariantPairListMetaTypeId = qRegisterMetaType<QVariantPairList>();

IDStreamer::IDStreamer(Bitstream& stream) :
    _stream(stream),
    _bits(1) {
}

static int getBitsForHighestValue(int highestValue) {
    // if this turns out to be a bottleneck, there are fancier ways to do it (get the position of the highest set bit):
    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    int bits = 0;
    while (highestValue != 0) {
        bits++;
        highestValue >>= 1;
    }
    return bits;
}

void IDStreamer::setBitsFromValue(int value) {
    _bits = getBitsForHighestValue(value + 1);
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
    streamer->_type = type;
    if (!streamer->_self) {
        streamer->_self = TypeStreamerPointer(streamer);
    }
    getTypeStreamers().insert(type, streamer);
    return 0;
}

const TypeStreamer* Bitstream::getTypeStreamer(int type) {
    return getTypeStreamers().value(type);
}

const QMetaObject* Bitstream::getMetaObject(const QByteArray& className) {
    return getMetaObjects().value(className);
}

QList<const QMetaObject*> Bitstream::getMetaObjectSubClasses(const QMetaObject* metaObject) {
    return getMetaObjectSubClasses().values(metaObject);
}

Bitstream::Bitstream(QDataStream& underlying, MetadataType metadataType, GenericsMode genericsMode, QObject* parent) :
    QObject(parent),
    _underlying(underlying),
    _byte(0),
    _position(0),
    _metadataType(metadataType),
    _genericsMode(genericsMode),
    _metaObjectStreamer(*this),
    _typeStreamerStreamer(*this),
    _attributeStreamer(*this),
    _scriptStringStreamer(*this),
    _sharedObjectStreamer(*this) {
}

void Bitstream::addMetaObjectSubstitution(const QByteArray& className, const QMetaObject* metaObject) {
    _metaObjectSubstitutions.insert(className, metaObject);
}

void Bitstream::addTypeSubstitution(const QByteArray& typeName, int type) {
    _typeStreamerSubstitutions.insert(typeName, getTypeStreamers().value(type));
}

void Bitstream::addTypeSubstitution(const QByteArray& typeName, const char* replacementTypeName) {
    const TypeStreamer* streamer = getTypeStreamers().value(QMetaType::type(replacementTypeName));
    if (!streamer) {
        streamer = getEnumStreamersByName().value(replacementTypeName);
    }
    _typeStreamerSubstitutions.insert(typeName, streamer);
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
        if (!it.key()) {
            continue;
        }
        connect(it.key().data(), SIGNAL(destroyed(QObject*)), SLOT(clearSharedObject(QObject*)));
        QPointer<SharedObject>& reference = _sharedObjectReferences[it.key()->getOriginID()];
        if (reference && reference != it.key()) {
            // the object has been replaced by a successor, so we can forget about the original
            _sharedObjectStreamer.removePersistentID(reference);
            reference->disconnect(this);
        }
        reference = it.key();
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
    
    for (QHash<int, SharedObjectPointer>::const_iterator it = mappings.sharedObjectValues.constBegin();
            it != mappings.sharedObjectValues.constEnd(); it++) {
        if (!it.value()) {
            continue;
        }
        QPointer<SharedObject>& reference = _sharedObjectReferences[it.value()->getRemoteOriginID()];
        if (reference && reference != it.value()) {
            // the object has been replaced by a successor, so we can forget about the original
            _sharedObjectStreamer.removePersistentValue(reference.data());
        }
        reference = it.value();
        _weakSharedObjectHash.remove(it.value()->getRemoteID());
    }
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

void Bitstream::writeDelta(bool value, bool reference) {
    *this << value;
}

void Bitstream::readDelta(bool& value, bool reference) {
    *this >> value;
}

void Bitstream::writeDelta(const QVariant& value, const QVariant& reference) {
    // QVariant only handles == for built-in types; we need to use our custom operators
    const TypeStreamer* streamer = getTypeStreamers().value(value.userType());
    if (value.userType() == reference.userType() && (!streamer || streamer->equal(value, reference))) {
        *this << false;
         return;
    }
    *this << true;
    _typeStreamerStreamer << streamer;
    streamer->writeRawDelta(*this, value, reference);
}

void Bitstream::writeRawDelta(const QVariant& value, const QVariant& reference) {
    const TypeStreamer* streamer = getTypeStreamers().value(value.userType());
    _typeStreamerStreamer << streamer;
    streamer->writeRawDelta(*this, value, reference);
}

void Bitstream::readRawDelta(QVariant& value, const QVariant& reference) {
    TypeStreamerPointer typeStreamer;
    _typeStreamerStreamer >> typeStreamer;
    typeStreamer->readRawDelta(*this, value, reference);
}

void Bitstream::writeRawDelta(const QObject* value, const QObject* reference) {
    if (!value) {
        _metaObjectStreamer << NULL;
        return;
    }
    const QMetaObject* metaObject = value->metaObject();
    _metaObjectStreamer << metaObject;
    foreach (const PropertyWriter& propertyWriter, getPropertyWriters().value(metaObject)) {
        propertyWriter.writeDelta(*this, value, reference);
    }
}

void Bitstream::readRawDelta(QObject*& value, const QObject* reference) {
    ObjectReader objectReader;
    _metaObjectStreamer >> objectReader;
    value = objectReader.readDelta(*this, reference);
}

void Bitstream::writeRawDelta(const QScriptValue& value, const QScriptValue& reference) {
    if (reference.isUndefined() || reference.isNull()) {
        *this << value;
    
    } else if (reference.isBool()) {
        if (value.isBool()) {
            *this << false;
            *this << value.toBool();
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isNumber()) {
        if (value.isNumber()) {
            *this << false;
            *this << value.toNumber();
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isString()) {
        if (value.isString()) {
            *this << false;
            *this << value.toString();
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isVariant()) {
        if (value.isVariant()) {
            *this << false;
            writeRawDelta(value.toVariant(), reference.toVariant());
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isQObject()) {
        if (value.isQObject()) {
            *this << false;
            writeRawDelta(value.toQObject(), reference.toQObject());
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isQMetaObject()) {
        if (value.isQMetaObject()) {
            *this << false;
            *this << value.toQMetaObject();
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isDate()) {
        if (value.isDate()) {
            *this << false;
            *this << value.toDateTime();
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isRegExp()) {
        if (value.isRegExp()) {
            *this << false;
            *this << value.toRegExp();
            
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isArray()) {
        if (value.isArray()) {
            *this << false;
            int length = value.property(ScriptCache::getInstance()->getLengthString()).toInt32();
            *this << length;
            int referenceLength = reference.property(ScriptCache::getInstance()->getLengthString()).toInt32();
            for (int i = 0; i < length; i++) {
                if (i < referenceLength) {
                    writeDelta(value.property(i), reference.property(i));
                } else {
                    *this << value.property(i);
                }
            }
        } else {
            *this << true;
            *this << value;
        }
    } else if (reference.isObject()) {
        if (value.isObject() && !(value.isArray() || value.isRegExp() || value.isDate() ||
                value.isQMetaObject() || value.isQObject() || value.isVariant())) {    
            *this << false;
            for (QScriptValueIterator it(value); it.hasNext(); ) {
                it.next();
                QScriptValue referenceValue = reference.property(it.scriptName());
                if (it.value() != referenceValue) {
                    *this << it.scriptName();
                    writeRawDelta(it.value(), referenceValue);
                }
            }
            for (QScriptValueIterator it(reference); it.hasNext(); ) {
                it.next();
                if (!value.property(it.scriptName()).isValid()) {
                    *this << it.scriptName();
                    writeRawDelta(QScriptValue(), it.value());
                }
            }
            *this << QScriptString();
            
        } else {
            *this << true;
            *this << value;
        }
    } else {
        *this << value;
    }
}

void Bitstream::readRawDelta(QScriptValue& value, const QScriptValue& reference) {
    if (reference.isUndefined() || reference.isNull()) {
        *this >> value;
    
    } else if (reference.isBool()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            bool boolValue;
            *this >> boolValue;
            value = QScriptValue(boolValue);
        }
    } else if (reference.isNumber()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            qsreal numberValue;
            *this >> numberValue;
            value = QScriptValue(numberValue);
        }
    } else if (reference.isString()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            QString stringValue;
            *this >> stringValue;
            value = QScriptValue(stringValue);
        }
    } else if (reference.isVariant()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            QVariant variant;
            readRawDelta(variant, reference.toVariant());
            value = ScriptCache::getInstance()->getEngine()->newVariant(variant);
        }
    } else if (reference.isQObject()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            QObject* object;
            readRawDelta(object, reference.toQObject());
            value = ScriptCache::getInstance()->getEngine()->newQObject(object, QScriptEngine::ScriptOwnership);
        }
    } else if (reference.isQMetaObject()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            const QMetaObject* metaObject;
            *this >> metaObject;
            value = ScriptCache::getInstance()->getEngine()->newQMetaObject(metaObject);
        }
    } else if (reference.isDate()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            QDateTime dateTime;
            *this >> dateTime;
            value = ScriptCache::getInstance()->getEngine()->newDate(dateTime);
        }
    } else if (reference.isRegExp()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            QRegExp regExp;
            *this >> regExp;
            value = ScriptCache::getInstance()->getEngine()->newRegExp(regExp);
        }
    } else if (reference.isArray()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            int length;
            *this >> length;
            value = ScriptCache::getInstance()->getEngine()->newArray(length);
            int referenceLength = reference.property(ScriptCache::getInstance()->getLengthString()).toInt32();
            for (int i = 0; i < length; i++) {
                QScriptValue element;
                if (i < referenceLength) {
                    readDelta(element, reference.property(i));
                } else {
                    *this >> element;
                }
                value.setProperty(i, element);
            }
        }
    } else if (reference.isObject()) {
        bool typeChanged;
        *this >> typeChanged;
        if (typeChanged) {
            *this >> value;
            
        } else {
            // start by shallow-copying the reference
            value = ScriptCache::getInstance()->getEngine()->newObject();
            for (QScriptValueIterator it(reference); it.hasNext(); ) {
                it.next();
                value.setProperty(it.scriptName(), it.value());
            }
            // then apply the requested changes
            forever {
                QScriptString name;
                *this >> name;
                if (!name.isValid()) {
                    break;
                }
                QScriptValue scriptValue;
                readRawDelta(scriptValue, reference.property(name));
                value.setProperty(name, scriptValue);
            }
        }
    } else {
        *this >> value;
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

Bitstream& Bitstream::operator<<(qint64 value) {
    return write(&value, 64);
}

Bitstream& Bitstream::operator>>(qint64& value) {
    return read(&value, 64);
}

Bitstream& Bitstream::operator<<(float value) {
    return write(&value, 32);
}

Bitstream& Bitstream::operator>>(float& value) {
    return read(&value, 32);
}

Bitstream& Bitstream::operator<<(double value) {
    return write(&value, 64);
}

Bitstream& Bitstream::operator>>(double& value) {
    return read(&value, 64);
}

Bitstream& Bitstream::operator<<(const glm::vec3& value) {
    return *this << value.x << value.y << value.z;
}

Bitstream& Bitstream::operator>>(glm::vec3& value) {
    return *this >> value.x >> value.y >> value.z;
}

Bitstream& Bitstream::operator<<(const glm::quat& value) {
    return *this << value.w << value.x << value.y << value.z;
}

Bitstream& Bitstream::operator>>(glm::quat& value) {
    return *this >> value.w >> value.x >> value.y >> value.z;
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

Bitstream& Bitstream::operator<<(const QDateTime& dateTime) {
    return *this << dateTime.toMSecsSinceEpoch();
}

Bitstream& Bitstream::operator>>(QDateTime& dateTime) {
    qint64 msecsSinceEpoch;
    *this >> msecsSinceEpoch;
    dateTime = QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch);
    return *this;
}

Bitstream& Bitstream::operator<<(const QRegExp& regExp) {
    *this << regExp.pattern();
    Qt::CaseSensitivity caseSensitivity = regExp.caseSensitivity();
    write(&caseSensitivity, 1);
    QRegExp::PatternSyntax syntax = regExp.patternSyntax();
    write(&syntax, 3);
    return *this << regExp.isMinimal();
}

Bitstream& Bitstream::operator>>(QRegExp& regExp) {
    QString pattern;
    *this >> pattern;
    Qt::CaseSensitivity caseSensitivity = (Qt::CaseSensitivity)0;
    read(&caseSensitivity, 1);
    QRegExp::PatternSyntax syntax = (QRegExp::PatternSyntax)0;
    read(&syntax, 3);
    regExp = QRegExp(pattern, caseSensitivity, syntax);
    bool minimal;
    *this >> minimal;
    regExp.setMinimal(minimal);
    return *this;
}

Bitstream& Bitstream::operator<<(const QVariant& value) {
    if (!value.isValid()) {
        _typeStreamerStreamer << NULL;
        return *this;
    }
    const TypeStreamer* streamer = getTypeStreamers().value(value.userType());
    if (streamer) {
        _typeStreamerStreamer << streamer->getStreamerToWrite(value);
        streamer->write(*this, value);
    } else {
        qWarning() << "Non-streamable type: " << value.typeName() << "\n";
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QVariant& value) {
    TypeStreamerPointer streamer;
    _typeStreamerStreamer >> streamer;
    if (!streamer) {
        value = QVariant();
    } else {
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

Bitstream& Bitstream::operator<<(const GenericValue& value) {
    value.getStreamer()->write(*this, value.getValue());
    return *this;
}

Bitstream& Bitstream::operator>>(GenericValue& value) {
    value = GenericValue();
    return *this;
}

Bitstream& Bitstream::operator<<(const QObject* object) {
    if (!object) {
        _metaObjectStreamer << NULL;
        return *this;
    }
    const QMetaObject* metaObject = object->metaObject();
    _metaObjectStreamer << metaObject;
    foreach (const PropertyWriter& propertyWriter, getPropertyWriters().value(metaObject)) {
        propertyWriter.write(*this, object);
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QObject*& object) {
    ObjectReader objectReader;
    _metaObjectStreamer >> objectReader;
    object = objectReader.read(*this);
    return *this;
}

Bitstream& Bitstream::operator<<(const QMetaObject* metaObject) {
    _metaObjectStreamer << metaObject;
    return *this;
}

Bitstream& Bitstream::operator>>(const QMetaObject*& metaObject) {
    ObjectReader objectReader;
    _metaObjectStreamer >> objectReader;
    metaObject = objectReader.getMetaObject();
    return *this;
}

Bitstream& Bitstream::operator>>(ObjectReader& objectReader) {
    _metaObjectStreamer >> objectReader;
    return *this;
}

Bitstream& Bitstream::operator<<(const TypeStreamer* streamer) {
    _typeStreamerStreamer << streamer;    
    return *this;
}

Bitstream& Bitstream::operator>>(const TypeStreamer*& streamer) {
    TypeStreamerPointer typeStreamer;
    _typeStreamerStreamer >> typeStreamer;
    streamer = typeStreamer.data();
    return *this;
}

Bitstream& Bitstream::operator>>(TypeStreamerPointer& streamer) {
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

enum ScriptValueType {
    INVALID_SCRIPT_VALUE,
    UNDEFINED_SCRIPT_VALUE,
    NULL_SCRIPT_VALUE,
    BOOL_SCRIPT_VALUE,
    NUMBER_SCRIPT_VALUE,
    STRING_SCRIPT_VALUE,
    VARIANT_SCRIPT_VALUE,
    QOBJECT_SCRIPT_VALUE,
    QMETAOBJECT_SCRIPT_VALUE,
    DATE_SCRIPT_VALUE,
    REGEXP_SCRIPT_VALUE,
    ARRAY_SCRIPT_VALUE,
    OBJECT_SCRIPT_VALUE
};

const int SCRIPT_VALUE_BITS = 4;

static void writeScriptValueType(Bitstream& out, ScriptValueType type) {
    out.write(&type, SCRIPT_VALUE_BITS);
}

static ScriptValueType readScriptValueType(Bitstream& in) {
    ScriptValueType type = (ScriptValueType)0;
    in.read(&type, SCRIPT_VALUE_BITS);
    return type;
}

Bitstream& Bitstream::operator<<(const QScriptValue& value) {
    if (value.isUndefined()) {
        writeScriptValueType(*this, UNDEFINED_SCRIPT_VALUE);
        
    } else if (value.isNull()) {
        writeScriptValueType(*this, NULL_SCRIPT_VALUE);
    
    } else if (value.isBool()) {
        writeScriptValueType(*this, BOOL_SCRIPT_VALUE);
        *this << value.toBool();
    
    } else if (value.isNumber()) {
        writeScriptValueType(*this, NUMBER_SCRIPT_VALUE);
        *this << value.toNumber();
    
    } else if (value.isString()) {
        writeScriptValueType(*this, STRING_SCRIPT_VALUE);
        *this << value.toString();
    
    } else if (value.isVariant()) {
        writeScriptValueType(*this, VARIANT_SCRIPT_VALUE);
        *this << value.toVariant();
        
    } else if (value.isQObject()) {
        writeScriptValueType(*this, QOBJECT_SCRIPT_VALUE);
        *this << value.toQObject();
    
    } else if (value.isQMetaObject()) {
        writeScriptValueType(*this, QMETAOBJECT_SCRIPT_VALUE);
        *this << value.toQMetaObject();
        
    } else if (value.isDate()) {
        writeScriptValueType(*this, DATE_SCRIPT_VALUE);
        *this << value.toDateTime();
    
    } else if (value.isRegExp()) {
        writeScriptValueType(*this, REGEXP_SCRIPT_VALUE);
        *this << value.toRegExp();
    
    } else if (value.isArray()) {
        writeScriptValueType(*this, ARRAY_SCRIPT_VALUE);
        int length = value.property(ScriptCache::getInstance()->getLengthString()).toInt32();
        *this << length;
        for (int i = 0; i < length; i++) {
            *this << value.property(i);
        }
    } else if (value.isObject()) {
        writeScriptValueType(*this, OBJECT_SCRIPT_VALUE);
        for (QScriptValueIterator it(value); it.hasNext(); ) {
            it.next();
            *this << it.scriptName();
            *this << it.value();
        }
        *this << QScriptString();
        
    } else {
        writeScriptValueType(*this, INVALID_SCRIPT_VALUE);
    }
    return *this;
}

Bitstream& Bitstream::operator>>(QScriptValue& value) {
    switch (readScriptValueType(*this)) {
        case UNDEFINED_SCRIPT_VALUE:
            value = QScriptValue(QScriptValue::UndefinedValue);
            break;
        
        case NULL_SCRIPT_VALUE:
            value = QScriptValue(QScriptValue::NullValue);
            break;
        
        case BOOL_SCRIPT_VALUE: {
            bool boolValue;
            *this >> boolValue;
            value = QScriptValue(boolValue);
            break;
        }
        case NUMBER_SCRIPT_VALUE: {
            qsreal numberValue;
            *this >> numberValue;
            value = QScriptValue(numberValue);
            break;
        }
        case STRING_SCRIPT_VALUE: {
            QString stringValue;
            *this >> stringValue;   
            value = QScriptValue(stringValue);
            break;
        }
        case VARIANT_SCRIPT_VALUE: {
            QVariant variantValue;
            *this >> variantValue;
            value = ScriptCache::getInstance()->getEngine()->newVariant(variantValue);
            break;
        }
        case QOBJECT_SCRIPT_VALUE: {
            QObject* object;
            *this >> object;
            ScriptCache::getInstance()->getEngine()->newQObject(object, QScriptEngine::ScriptOwnership);
            break;
        }
        case QMETAOBJECT_SCRIPT_VALUE: {
            const QMetaObject* metaObject;
            *this >> metaObject;
            ScriptCache::getInstance()->getEngine()->newQMetaObject(metaObject);
            break;
        }
        case DATE_SCRIPT_VALUE: {
            QDateTime dateTime;
            *this >> dateTime;
            value = ScriptCache::getInstance()->getEngine()->newDate(dateTime);
            break;
        }
        case REGEXP_SCRIPT_VALUE: {
            QRegExp regExp;
            *this >> regExp;
            value = ScriptCache::getInstance()->getEngine()->newRegExp(regExp);
            break;
        }
        case ARRAY_SCRIPT_VALUE: {
            int length;
            *this >> length;
            value = ScriptCache::getInstance()->getEngine()->newArray(length);
            for (int i = 0; i < length; i++) {
                QScriptValue element;
                *this >> element;
                value.setProperty(i, element);
            }
            break;
        }
        case OBJECT_SCRIPT_VALUE: {
            value = ScriptCache::getInstance()->getEngine()->newObject();
            forever {
                QScriptString name;
                *this >> name;
                if (!name.isValid()) {
                    break;
                }
                QScriptValue scriptValue;
                *this >> scriptValue;
                value.setProperty(name, scriptValue);
            }
            break;
        }
        default:
            value = QScriptValue();
            break;
    }
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
    if (!metaObject) {
        return *this << QByteArray();
    }
    *this << QByteArray::fromRawData(metaObject->className(), strlen(metaObject->className()));
    if (_metadataType == NO_METADATA) {
        return *this;
    }
    const PropertyWriterVector& propertyWriters = getPropertyWriters().value(metaObject);
    *this << propertyWriters.size();
    QCryptographicHash hash(QCryptographicHash::Md5);
    foreach (const PropertyWriter& propertyWriter, propertyWriters) {
        _typeStreamerStreamer << propertyWriter.getStreamer();
        const QMetaProperty& property = propertyWriter.getProperty();
        if (_metadataType == FULL_METADATA) {
            *this << QByteArray::fromRawData(property.name(), strlen(property.name()));
        } else {
            hash.addData(property.name(), strlen(property.name()) + 1);
        }
    }
    if (_metadataType == HASH_METADATA) {
        QByteArray hashResult = hash.result();
        write(hashResult.constData(), hashResult.size() * BITS_IN_BYTE);
    }
    return *this;
}

Bitstream& Bitstream::operator>(ObjectReader& objectReader) {
    QByteArray className;
    *this >> className;
    if (className.isEmpty()) {
        objectReader = ObjectReader();
        return *this;
    }
    const QMetaObject* metaObject = _metaObjectSubstitutions.value(className);
    if (!metaObject) {
        metaObject = getMetaObjects().value(className);
    }
    if (!metaObject) {
        qWarning() << "Unknown class name: " << className << "\n";
    }
    if (_metadataType == NO_METADATA) {
        objectReader = ObjectReader(className, metaObject, getPropertyReaders().value(metaObject));
        return *this;
    }
    int storedPropertyCount;
    *this >> storedPropertyCount;
    PropertyReaderVector properties(storedPropertyCount);
    for (int i = 0; i < storedPropertyCount; i++) {
        TypeStreamerPointer typeStreamer;
        *this >> typeStreamer;
        QMetaProperty property = QMetaProperty();
        if (_metadataType == FULL_METADATA) {
            QByteArray propertyName;
            *this >> propertyName;
            if (metaObject) {
                property = metaObject->property(metaObject->indexOfProperty(propertyName));
            }
        }
        properties[i] = PropertyReader(typeStreamer, property);
    }
    // for hash metadata, check the names/types of the properties as well as the name hash against our own class
    if (_metadataType == HASH_METADATA) {
        QCryptographicHash hash(QCryptographicHash::Md5);
        bool matches = true;
        if (metaObject) {
            const PropertyWriterVector& propertyWriters = getPropertyWriters().value(metaObject);
            if (propertyWriters.size() == properties.size()) {
                for (int i = 0; i < propertyWriters.size(); i++) {
                    const PropertyWriter& propertyWriter = propertyWriters.at(i);
                    if (properties.at(i).getStreamer() != propertyWriter.getStreamer()) {
                        matches = false;
                        break;
                    }
                    const QMetaProperty& property = propertyWriter.getProperty();
                    hash.addData(property.name(), strlen(property.name()) + 1); 
                }
            } else {
                matches = false;
            }
        }
        QByteArray localHashResult = hash.result();
        QByteArray remoteHashResult(localHashResult.size(), 0);
        read(remoteHashResult.data(), remoteHashResult.size() * BITS_IN_BYTE);
        if (metaObject && matches && localHashResult == remoteHashResult) {
            objectReader = ObjectReader(className, metaObject, getPropertyReaders().value(metaObject));
            return *this;
        }
    }
    objectReader = ObjectReader(className, metaObject, properties);
    return *this;
}

Bitstream& Bitstream::operator<(const TypeStreamer* streamer) {
    if (!streamer) {
        *this << QByteArray();
        return *this;
    }
    const char* typeName = streamer->getName();
    *this << QByteArray::fromRawData(typeName, strlen(typeName));
    if (_metadataType != NO_METADATA) {
        *this << (int)streamer->getCategory();
        streamer->writeMetadata(*this, _metadataType == FULL_METADATA);
    } 
    return *this;
}

Bitstream& Bitstream::operator>(TypeStreamerPointer& streamer) {
    QByteArray typeName;
    *this >> typeName;
    if (typeName.isEmpty()) {
        streamer = TypeStreamerPointer();
        return *this;
    }
    const TypeStreamer* baseStreamer = _typeStreamerSubstitutions.value(typeName);
    if (!baseStreamer) {
        baseStreamer = getTypeStreamers().value(QMetaType::type(typeName.constData()));
        if (!baseStreamer) {
            baseStreamer = getEnumStreamersByName().value(typeName);
        }
    }
    // start out with the base, if any
    if (baseStreamer) {
        streamer = baseStreamer->getSelf();
    } else {
        streamer = TypeStreamerPointer();
    }
    if (_metadataType == NO_METADATA) {
        if (!baseStreamer) {
            qWarning() << "Unknown type name:" << typeName;
        }
        return *this;
    }
    int category;
    *this >> category;
    if (category == TypeStreamer::SIMPLE_CATEGORY) {
        if (!streamer) {
            qWarning() << "Unknown type name:" << typeName;
        }
        return *this;
    }
    if (_genericsMode == ALL_GENERICS) {
        streamer = readGenericTypeStreamer(typeName, category);
        return *this;
    }
    if (!baseStreamer) {
        if (_genericsMode == FALLBACK_GENERICS) {
            streamer = readGenericTypeStreamer(typeName, category);
            return *this;
        }
        baseStreamer = getInvalidTypeStreamer();
    }
    switch (category) {
        case TypeStreamer::ENUM_CATEGORY: {
            if (_metadataType == FULL_METADATA) {
                int keyCount;
                *this >> keyCount;
                QMetaEnum metaEnum = baseStreamer->getMetaEnum();
                QHash<int, int> mappings;
                bool matches = (keyCount == metaEnum.keyCount());
                int highestValue = 0;
                for (int i = 0; i < keyCount; i++) {
                    QByteArray key;
                    int value;
                    *this >> key >> value;
                    highestValue = qMax(value, highestValue);
                    int localValue = metaEnum.keyToValue(key);
                    if (localValue != -1) {
                        mappings.insert(value, localValue);
                    }
                    matches &= (value == localValue);
                }
                if (!matches) {
                    streamer = TypeStreamerPointer(new MappedEnumTypeStreamer(baseStreamer,
                        getBitsForHighestValue(highestValue), mappings));
                }
            } else {
                int bits;
                *this >> bits;
                QCryptographicHash hash(QCryptographicHash::Md5);
                if (baseStreamer->getCategory() == TypeStreamer::ENUM_CATEGORY) {
                    QMetaEnum metaEnum = baseStreamer->getMetaEnum();
                    for (int i = 0; i < metaEnum.keyCount(); i++) {
                        hash.addData(metaEnum.key(i), strlen(metaEnum.key(i)) + 1);
                        qint32 value = metaEnum.value(i);
                        hash.addData((const char*)&value, sizeof(qint32));
                    }
                }
                QByteArray localHashResult = hash.result();
                QByteArray remoteHashResult(localHashResult.size(), 0);
                read(remoteHashResult.data(), remoteHashResult.size() * BITS_IN_BYTE);
                if (localHashResult != remoteHashResult) {
                    streamer = TypeStreamerPointer(new MappedEnumTypeStreamer(baseStreamer, bits, QHash<int, int>()));
                }
            }
            return *this;
        }
        case TypeStreamer::LIST_CATEGORY:
        case TypeStreamer::SET_CATEGORY: {
            TypeStreamerPointer valueStreamer;
            *this >> valueStreamer;
            if (!(baseStreamer->getCategory() == category && valueStreamer == baseStreamer->getValueStreamer())) {
                streamer = TypeStreamerPointer(category == TypeStreamer::LIST_CATEGORY ?
                    new MappedListTypeStreamer(baseStreamer, valueStreamer) :
                        new MappedSetTypeStreamer(baseStreamer, valueStreamer));
            }
            return *this;
        }
        case TypeStreamer::MAP_CATEGORY: {
            TypeStreamerPointer keyStreamer, valueStreamer;
            *this >> keyStreamer >> valueStreamer;
            if (!(baseStreamer->getCategory() == TypeStreamer::MAP_CATEGORY &&
                    keyStreamer == baseStreamer->getKeyStreamer() && valueStreamer == baseStreamer->getValueStreamer())) {
                streamer = TypeStreamerPointer(new MappedMapTypeStreamer(baseStreamer, keyStreamer, valueStreamer));
            }
            return *this;
        }
    }
    // streamable type
    int fieldCount;
    *this >> fieldCount;
    QVector<StreamerIndexPair> fields(fieldCount);
    for (int i = 0; i < fieldCount; i++) {
        TypeStreamerPointer typeStreamer;
        *this >> typeStreamer;
        int index = -1;
        if (_metadataType == FULL_METADATA) {
            QByteArray fieldName;
            *this >> fieldName;
            index = baseStreamer->getFieldIndex(fieldName);
        }
        fields[i] = StreamerIndexPair(typeStreamer, index);
    }
    // for hash metadata, check the names/types of the fields as well as the name hash against our own class
    if (_metadataType == HASH_METADATA) {
        QCryptographicHash hash(QCryptographicHash::Md5);
        bool matches = true;
        const QVector<MetaField>& localFields = baseStreamer->getMetaFields();
        if (fieldCount != localFields.size()) {
            matches = false;
            
        } else {
            if (fieldCount == 0) {
                return *this;
            }
            for (int i = 0; i < fieldCount; i++) {
                const MetaField& localField = localFields.at(i);
                if (fields.at(i).first != localField.getStreamer()) {
                    matches = false;
                    break;
                }
                hash.addData(localField.getName().constData(), localField.getName().size() + 1);
            }   
        }
        QByteArray localHashResult = hash.result();
        QByteArray remoteHashResult(localHashResult.size(), 0);
        read(remoteHashResult.data(), remoteHashResult.size() * BITS_IN_BYTE);
        if (matches && localHashResult == remoteHashResult) {
            // since everything is the same, we can use the default streamer
            return *this;
        }
    }
    // if all fields are the same type and in the right order, we can use the (more efficient) default streamer
    const QVector<MetaField>& localFields = baseStreamer->getMetaFields();
    if (fieldCount != localFields.size()) {
        streamer = TypeStreamerPointer(new MappedStreamableTypeStreamer(baseStreamer, fields));
        return *this;
    }
    for (int i = 0; i < fieldCount; i++) {
        const StreamerIndexPair& field = fields.at(i);
        if (field.first != localFields.at(i).getStreamer() || field.second != i) {
            streamer = TypeStreamerPointer(new MappedStreamableTypeStreamer(baseStreamer, fields));
            return *this;
        }
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

const QString INVALID_STRING("%INVALID%");

Bitstream& Bitstream::operator<(const QScriptString& string) {
    return *this << (string.isValid() ? string.toString() : INVALID_STRING);
}

Bitstream& Bitstream::operator>(QScriptString& string) {
    QString rawString;
    *this >> rawString;
    string = (rawString == INVALID_STRING) ? QScriptString() :
        ScriptCache::getInstance()->getEngine()->toStringHandle(rawString);
    return *this;
}

Bitstream& Bitstream::operator<(const SharedObjectPointer& object) {
    if (!object) {
        return *this << (int)0;
    }
    *this << object->getID();
    *this << object->getOriginID();
    QPointer<SharedObject> reference = _sharedObjectReferences.value(object->getOriginID());
    if (reference) {
        writeRawDelta((const QObject*)object.data(), (const QObject*)reference.data());
    } else {
        *this << (QObject*)object.data();
    }
    return *this;
}

Bitstream& Bitstream::operator>(SharedObjectPointer& object) {
    int id;
    *this >> id;
    if (id == 0) {
        object = SharedObjectPointer();
        return *this;
    }
    int originID;
    *this >> originID;
    QPointer<SharedObject> reference = _sharedObjectReferences.value(originID);
    QPointer<SharedObject>& pointer = _weakSharedObjectHash[id];
    if (pointer) {
        ObjectReader objectReader;
        _metaObjectStreamer >> objectReader;
        if (reference) {
            objectReader.readDelta(*this, reference.data(), pointer.data());
        } else {
            objectReader.read(*this, pointer.data());
        }
    } else {
        QObject* rawObject; 
        if (reference) {
            readRawDelta(rawObject, (const QObject*)reference.data());
        } else {
            *this >> rawObject;
        }
        pointer = static_cast<SharedObject*>(rawObject);
        if (pointer) {
            if (reference) {
                pointer->setOriginID(reference->getOriginID());
            }
            pointer->setRemoteID(id);
            pointer->setRemoteOriginID(originID);
        } else {
            qDebug() << "Null object" << pointer << reference << id;
        }
    }
    object = static_cast<SharedObject*>(pointer.data());
    return *this;
}

void Bitstream::clearSharedObject(QObject* object) {
    SharedObject* sharedObject = static_cast<SharedObject*>(object);
    _sharedObjectReferences.remove(sharedObject->getOriginID());
    int id = _sharedObjectStreamer.takePersistentID(sharedObject);
    if (id != 0) {
        emit sharedObjectCleared(id);
    }
}

const int MD5_HASH_SIZE = 16;

TypeStreamerPointer Bitstream::readGenericTypeStreamer(const QByteArray& name, int category) {
    TypeStreamerPointer streamer;
    switch (category) {
        case TypeStreamer::ENUM_CATEGORY: {
            QVector<NameIntPair> values;
            int bits;
            QByteArray hash;
            if (_metadataType == FULL_METADATA) {
                int keyCount;
                *this >> keyCount;
                values.resize(keyCount);
                int highestValue = 0;
                for (int i = 0; i < keyCount; i++) {
                    QByteArray name;
                    int value;
                    *this >> name >> value;
                    values[i] = NameIntPair(name, value);
                    highestValue = qMax(highestValue, value);
                }
                bits = getBitsForHighestValue(highestValue);
                
            } else {
                *this >> bits;
                hash.resize(MD5_HASH_SIZE);
                read(hash.data(), hash.size() * BITS_IN_BYTE);
            }
            streamer = TypeStreamerPointer(new GenericEnumTypeStreamer(name, values, bits, hash));
            break;
        }
        case TypeStreamer::STREAMABLE_CATEGORY: {
            int fieldCount;
            *this >> fieldCount;
            QVector<StreamerNamePair> fields(fieldCount);
            QByteArray hash;
            if (fieldCount == 0) {
                streamer = TypeStreamerPointer(new GenericStreamableTypeStreamer(name, fields, hash));
                break;
            }
            for (int i = 0; i < fieldCount; i++) {
                TypeStreamerPointer streamer;
                *this >> streamer;
                QByteArray name;
                if (_metadataType == FULL_METADATA) {
                    *this >> name;
                }
                fields[i] = StreamerNamePair(streamer, name);
            }
            if (_metadataType == HASH_METADATA) {
                hash.resize(MD5_HASH_SIZE);
                read(hash.data(), hash.size() * BITS_IN_BYTE);
            }
            streamer = TypeStreamerPointer(new GenericStreamableTypeStreamer(name, fields, hash));
            break;
        }
        case TypeStreamer::LIST_CATEGORY:
        case TypeStreamer::SET_CATEGORY: {
            TypeStreamerPointer valueStreamer;
            *this >> valueStreamer;
            streamer = TypeStreamerPointer(category == TypeStreamer::LIST_CATEGORY ?
                new GenericListTypeStreamer(name, valueStreamer) : new GenericSetTypeStreamer(name, valueStreamer));
            break;
        }
        case TypeStreamer::MAP_CATEGORY: {
            TypeStreamerPointer keyStreamer, valueStreamer;
            *this >> keyStreamer >> valueStreamer;
            streamer = TypeStreamerPointer(new GenericMapTypeStreamer(name, keyStreamer, valueStreamer));
            break;
        }
    }
    static_cast<GenericTypeStreamer*>(streamer.data())->_weakSelf = streamer;
    return streamer;
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

const QHash<ScopeNamePair, const TypeStreamer*>& Bitstream::getEnumStreamers() {
    static QHash<ScopeNamePair, const TypeStreamer*> enumStreamers = createEnumStreamers();
    return enumStreamers;
}

QHash<ScopeNamePair, const TypeStreamer*> Bitstream::createEnumStreamers() {
    QHash<ScopeNamePair, const TypeStreamer*> enumStreamers;
    foreach (const QMetaObject* metaObject, getMetaObjects()) {
        for (int i = 0; i < metaObject->enumeratorCount(); i++) {
            QMetaEnum metaEnum = metaObject->enumerator(i);
            const TypeStreamer*& streamer = enumStreamers[ScopeNamePair(metaEnum.scope(), metaEnum.name())];
            if (!streamer) {
                streamer = new EnumTypeStreamer(metaEnum);
            }
        }
    }
    return enumStreamers;
}

const QHash<QByteArray, const TypeStreamer*>& Bitstream::getEnumStreamersByName() {
    static QHash<QByteArray, const TypeStreamer*> enumStreamersByName = createEnumStreamersByName();
    return enumStreamersByName;
}

QHash<QByteArray, const TypeStreamer*> Bitstream::createEnumStreamersByName() {
    QHash<QByteArray, const TypeStreamer*> enumStreamersByName;
    foreach (const TypeStreamer* streamer, getEnumStreamers()) {
        enumStreamersByName.insert(streamer->getName(), streamer);
    }
    return enumStreamersByName;
}

const TypeStreamer* Bitstream::getInvalidTypeStreamer() {
    const TypeStreamer* streamer = createInvalidTypeStreamer();
    return streamer;
}

const TypeStreamer* Bitstream::createInvalidTypeStreamer() {
    TypeStreamer* streamer = new TypeStreamer();
    streamer->_type = QMetaType::UnknownType;
    streamer->_self = TypeStreamerPointer(streamer);
    return streamer;
}

const QHash<const QMetaObject*, PropertyReaderVector>& Bitstream::getPropertyReaders() {
    static QHash<const QMetaObject*, PropertyReaderVector> propertyReaders = createPropertyReaders();
    return propertyReaders;
}

QHash<const QMetaObject*, PropertyReaderVector> Bitstream::createPropertyReaders() {
    QHash<const QMetaObject*, PropertyReaderVector> propertyReaders;
    foreach (const QMetaObject* metaObject, getMetaObjects()) {
        PropertyReaderVector& readers = propertyReaders[metaObject];
        for (int i = 0; i < metaObject->propertyCount(); i++) {
            QMetaProperty property = metaObject->property(i);
            if (!property.isStored()) {
                continue;
            }
            const TypeStreamer* streamer;
            if (property.isEnumType()) {
                QMetaEnum metaEnum = property.enumerator();
                streamer = getEnumStreamers().value(ScopeNamePair(
                    QByteArray::fromRawData(metaEnum.scope(), strlen(metaEnum.scope())),
                    QByteArray::fromRawData(metaEnum.name(), strlen(metaEnum.name()))));
            } else {
                streamer = getTypeStreamers().value(property.userType());    
            }
            if (streamer) {
                readers.append(PropertyReader(streamer->getSelf(), property));
            }
        }
    }
    return propertyReaders;
}

const QHash<const QMetaObject*, PropertyWriterVector>& Bitstream::getPropertyWriters() {
    static QHash<const QMetaObject*, PropertyWriterVector> propertyWriters = createPropertyWriters();
    return propertyWriters;
}

QHash<const QMetaObject*, PropertyWriterVector> Bitstream::createPropertyWriters() {
    QHash<const QMetaObject*, PropertyWriterVector> propertyWriters;
    foreach (const QMetaObject* metaObject, getMetaObjects()) {
        PropertyWriterVector& writers = propertyWriters[metaObject];
        for (int i = 0; i < metaObject->propertyCount(); i++) {
            QMetaProperty property = metaObject->property(i);
            if (!property.isStored()) {
                continue;
            }
            const TypeStreamer* streamer;
            if (property.isEnumType()) {
                QMetaEnum metaEnum = property.enumerator();
                streamer = getEnumStreamers().value(ScopeNamePair(
                    QByteArray::fromRawData(metaEnum.scope(), strlen(metaEnum.scope())),
                    QByteArray::fromRawData(metaEnum.name(), strlen(metaEnum.name()))));
            } else {
                streamer = getTypeStreamers().value(property.userType());    
            }
            if (streamer) {
                writers.append(PropertyWriter(property, streamer));
            }
        }
    }
    return propertyWriters;
}

MappedObjectStreamer::MappedObjectStreamer(const QVector<StreamerPropertyPair>& properties) :
    _properties(properties) {
}

const char* MappedObjectStreamer::getName() const {
    return _metaObject->className();
}

void MappedObjectStreamer::writeMetadata(Bitstream& out, bool full) const {
    out << _properties.size();
    if (_properties.isEmpty()) {
        return;
    }
    QCryptographicHash hash(QCryptographicHash::Md5);
    foreach (const StreamerPropertyPair& property, _properties) {
        out << property.first.data();
        if (full) {
            out << QByteArray::fromRawData(property.second.name(), strlen(property.second.name()));
        } else {
            hash.addData(property.second.name(), strlen(property.second.name()) + 1);
        }
    }
    if (!full) {
        QByteArray hashResult = hash.result();
        out.write(hashResult.constData(), hashResult.size() * BITS_IN_BYTE);
    }
}

void MappedObjectStreamer::write(Bitstream& out, QObject* object) const {
    foreach (const StreamerPropertyPair& property, _properties) {
        property.first->write(out, property.second.read(object));
    }
}

void MappedObjectStreamer::writeDelta(Bitstream& out, QObject* object, QObject* reference) const {
    foreach (const StreamerPropertyPair& property, _properties) {
        property.first->writeDelta(out, property.second.read(object), (reference && reference->metaObject() == _metaObject) ?
            property.second.read(reference) : QVariant());
    }
}

QObject* MappedObjectStreamer::read(Bitstream& in, QObject* object) const {
    if (!object && _metaObject) {
        object = _metaObject->newInstance();
    }
    foreach (const StreamerPropertyPair& property, _properties) {
        QVariant value = property.first->read(in);
        if (property.second.isValid() && object) {
            property.second.write(object, value);
        }
    }
    return object;
}

QObject* MappedObjectStreamer::readDelta(Bitstream& in, const QObject* reference, QObject* object) const {
    if (!object && _metaObject) {
        object = _metaObject->newInstance();
    }
    foreach (const StreamerPropertyPair& property, _properties) {
        QVariant value;
        property.first->readDelta(in, value, (property.second.isValid() && reference) ?
            property.second.read(reference) : QVariant());
        if (property.second.isValid() && object) {
            property.second.write(object, value);
        }
    }
    return object;
}

GenericObjectStreamer::GenericObjectStreamer(const QByteArray& name, const QVector<StreamerNamePair>& properties,
        const QByteArray& hash) :
    _name(name),
    _properties(properties),
    _hash(hash) {
    
    _metaObject = &GenericSharedObject::staticMetaObject;
}

const char* GenericObjectStreamer::getName() const {
    return _name.constData();
}

void GenericObjectStreamer::writeMetadata(Bitstream& out, bool full) const {
    out << _properties.size();
    if (_properties.isEmpty()) {
        return;
    }
    foreach (const StreamerNamePair& property, _properties) {
        out << property.first.data();
        if (full) {
            out << property.second;
        }
    }
    if (!full) {
        if (_hash.isEmpty()) {
            QCryptographicHash hash(QCryptographicHash::Md5);
            foreach (const StreamerNamePair& property, _properties) {
                hash.addData(property.second.constData(), property.second.size() + 1);
            }
            const_cast<GenericObjectStreamer*>(this)->_hash = hash.result();
        }
        out.write(_hash.constData(), _hash.size() * BITS_IN_BYTE);
    }
}

void GenericObjectStreamer::write(Bitstream& out, QObject* object) const {
    const QVariantList& values = static_cast<GenericSharedObject*>(object)->getValues();
    for (int i = 0; i < _properties.size(); i++) {
        _properties.at(i).first->write(out, values.at(i));
    }
}

void GenericObjectStreamer::writeDelta(Bitstream& out, QObject* object, QObject* reference) const {
    for (int i = 0; i < _properties.size(); i++) {
        _properties.at(i).first->writeDelta(out, static_cast<GenericSharedObject*>(object)->getValues().at(i), reference ?
            static_cast<GenericSharedObject*>(reference)->getValues().at(i) : QVariant());
    }
}

QObject* GenericObjectStreamer::read(Bitstream& in, QObject* object) const {
    if (!object) {
        object = new GenericSharedObject(_weakSelf);
    }
    QVariantList values;
    foreach (const StreamerNamePair& property, _properties) {
        values.append(property.first->read(in));
    }
    static_cast<GenericSharedObject*>(object)->setValues(values);
    return object;
}

QObject* GenericObjectStreamer::readDelta(Bitstream& in, const QObject* reference, QObject* object) const {
    if (!object) {
        object = new GenericSharedObject(_weakSelf);
    }
    QVariantList values;
    for (int i = 0; i < _properties.size(); i++) {
        const StreamerNamePair& property = _properties.at(i);
        QVariant value;
        property.first->readDelta(in, value, reference ?
            static_cast<const GenericSharedObject*>(reference)->getValues().at(i) : QVariant());
        values.append(value);
    }
    static_cast<GenericSharedObject*>(object)->setValues(values);
    return object;
}

ObjectReader::ObjectReader(const QByteArray& className, const QMetaObject* metaObject,
        const PropertyReaderVector& properties) :
    _className(className),
    _metaObject(metaObject),
    _properties(properties) {
}

QObject* ObjectReader::read(Bitstream& in, QObject* object) const {
    if (!object && _metaObject) {
        object = _metaObject->newInstance();
    }
    foreach (const PropertyReader& property, _properties) {
        property.read(in, object);
    }
    return object;
}

QObject* ObjectReader::readDelta(Bitstream& in, const QObject* reference, QObject* object) const {
    if (!object && _metaObject) {
        object = _metaObject->newInstance();
    }
    foreach (const PropertyReader& property, _properties) {
        property.readDelta(in, object, reference);
    }
    return object;
}

uint qHash(const ObjectReader& objectReader, uint seed) {
    return qHash(objectReader.getClassName(), seed);
}

QDebug& operator<<(QDebug& debug, const ObjectReader& objectReader) {
    return debug << objectReader.getClassName();
}

PropertyReader::PropertyReader(const TypeStreamerPointer& streamer, const QMetaProperty& property) :
    _streamer(streamer),
    _property(property) {
}

void PropertyReader::read(Bitstream& in, QObject* object) const {
    QVariant value = _streamer->read(in);
    if (_property.isValid() && object) {
        _property.write(object, value);
    }
}

void PropertyReader::readDelta(Bitstream& in, QObject* object, const QObject* reference) const {
    QVariant value;
    _streamer->readDelta(in, value, (_property.isValid() && reference) ? _property.read(reference) : QVariant());
    if (_property.isValid() && object) {
        _property.write(object, value);
    }
}

PropertyWriter::PropertyWriter(const QMetaProperty& property, const TypeStreamer* streamer) :
    _property(property),
    _streamer(streamer) {
}

void PropertyWriter::write(Bitstream& out, const QObject* object) const {
    _streamer->write(out, _property.read(object));
}

void PropertyWriter::writeDelta(Bitstream& out, const QObject* object, const QObject* reference) const {
    _streamer->writeDelta(out, _property.read(object), reference && object->metaObject() == reference->metaObject() ?
        _property.read(reference) : QVariant());
}

MetaField::MetaField(const QByteArray& name, const TypeStreamer* streamer) :
    _name(name),
    _streamer(streamer) {
}

GenericValue::GenericValue(const TypeStreamerPointer& streamer, const QVariant& value) :
    _streamer(streamer),
    _value(value) {
}

bool GenericValue::operator==(const GenericValue& other) const {
    return _streamer == other._streamer && _value == other._value;
}

GenericSharedObject::GenericSharedObject(const ObjectStreamerPointer& streamer) :
    _streamer(streamer) {
}

TypeStreamer::~TypeStreamer() {
}

const char* TypeStreamer::getName() const {
    return QMetaType::typeName(_type);
}

const TypeStreamer* TypeStreamer::getStreamerToWrite(const QVariant& value) const {
    return this;
}

void TypeStreamer::writeMetadata(Bitstream& out, bool full) const {
    if (getCategory() != STREAMABLE_CATEGORY) {
        return;
    }
    // streamable type
    const QVector<MetaField>& metaFields = getMetaFields();
    out << metaFields.size();
    if (metaFields.isEmpty()) {
        return;
    }
    QCryptographicHash hash(QCryptographicHash::Md5);
    foreach (const MetaField& metaField, metaFields) {
        out << metaField.getStreamer();
        if (full) {
            out << metaField.getName();
        } else {
            hash.addData(metaField.getName().constData(), metaField.getName().size() + 1);
        }
    }
    if (!full) {
        QByteArray hashResult = hash.result();
        out.write(hashResult.constData(), hashResult.size() * BITS_IN_BYTE);
    }
}

bool TypeStreamer::equal(const QVariant& first, const QVariant& second) const {
    return first == second;
}

void TypeStreamer::write(Bitstream& out, const QVariant& value) const {
    // nothing by default
}

QVariant TypeStreamer::read(Bitstream& in) const {
    return QVariant();
}

void TypeStreamer::writeDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const {
    if (value == reference) {
        out << false;
    } else {
        out << true;
        writeRawDelta(out, value, reference);
    }
}

void TypeStreamer::readDelta(Bitstream& in, QVariant& value, const QVariant& reference) const {
    bool changed;
    in >> changed;
    if (changed) {
        readRawDelta(in, value, reference);
    } else {
        value = reference;
    }
}

void TypeStreamer::writeRawDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const {
    // nothing by default
}

void TypeStreamer::readRawDelta(Bitstream& in, QVariant& value, const QVariant& reference) const {
    value = reference;
}

void TypeStreamer::setEnumValue(QVariant& object, int value, const QHash<int, int>& mappings) const {
    // nothing by default
}

const QVector<MetaField>& TypeStreamer::getMetaFields() const {
    static QVector<MetaField> emptyMetaFields;
    return emptyMetaFields;
}

int TypeStreamer::getFieldIndex(const QByteArray& name) const {
    return -1;
}

void TypeStreamer::setField(QVariant& object, int index, const QVariant& value) const {
    // nothing by default
}

QVariant TypeStreamer::getField(const QVariant& object, int index) const {
    return QVariant();
}

TypeStreamer::Category TypeStreamer::getCategory() const {
    return SIMPLE_CATEGORY;
}

int TypeStreamer::getBits() const {
    return 0;
}

QMetaEnum TypeStreamer::getMetaEnum() const {
    return QMetaEnum();
}

const TypeStreamer* TypeStreamer::getKeyStreamer() const {
    return NULL;
}

const TypeStreamer* TypeStreamer::getValueStreamer() const {
    return NULL;
}

void TypeStreamer::insert(QVariant& object, const QVariant& element) const {
    // nothing by default
}

void TypeStreamer::insert(QVariant& object, const QVariant& key, const QVariant& value) const {
    // nothing by default
}

bool TypeStreamer::remove(QVariant& object, const QVariant& key) const {
    return false;
}

QVariant TypeStreamer::getValue(const QVariant& object, const QVariant& key) const {
    return QVariant();
}

void TypeStreamer::prune(QVariant& object, int size) const {
    // nothing by default
}

QVariant TypeStreamer::getValue(const QVariant& object, int index) const {
    return QVariant();
}

void TypeStreamer::setValue(QVariant& object, int index, const QVariant& value) const {
    // nothing by default
}

QDebug& operator<<(QDebug& debug, const TypeStreamer* typeStreamer) {
    return debug << (typeStreamer ? QMetaType::typeName(typeStreamer->getType()) : "null");
}

QDebug& operator<<(QDebug& debug, const QMetaObject* metaObject) {
    return debug << (metaObject ? metaObject->className() : "null");
}

EnumTypeStreamer::EnumTypeStreamer(const QMetaObject* metaObject, const char* name) :
    _metaObject(metaObject),
    _enumName(name),
    _name(QByteArray(metaObject->className()) + "::" + name),
    _bits(-1) {
    
    _type = QMetaType::Int;
    _self = TypeStreamerPointer(this);
}

EnumTypeStreamer::EnumTypeStreamer(const QMetaEnum& metaEnum) :
    _name(QByteArray(metaEnum.scope()) + "::" + metaEnum.name()),
    _metaEnum(metaEnum),
    _bits(-1) {
    
    _type = QMetaType::Int;
    _self = TypeStreamerPointer(this);
}

const char* EnumTypeStreamer::getName() const {
    return _name.constData();
}

void EnumTypeStreamer::writeMetadata(Bitstream& out, bool full) const {
    QMetaEnum metaEnum = getMetaEnum();
    if (full) {
        out << metaEnum.keyCount();
        for (int i = 0; i < metaEnum.keyCount(); i++) {
            out << QByteArray::fromRawData(metaEnum.key(i), strlen(metaEnum.key(i)));
            out << metaEnum.value(i);
        }
    } else {
        out << getBits();
        QCryptographicHash hash(QCryptographicHash::Md5);    
        for (int i = 0; i < metaEnum.keyCount(); i++) {
            hash.addData(metaEnum.key(i), strlen(metaEnum.key(i)) + 1);
            qint32 value = metaEnum.value(i);
            hash.addData((const char*)&value, sizeof(qint32));
        }
        QByteArray hashResult = hash.result();
        out.write(hashResult.constData(), hashResult.size() * BITS_IN_BYTE);
    }
}

TypeStreamer::Category EnumTypeStreamer::getCategory() const {
    return ENUM_CATEGORY;
}

int EnumTypeStreamer::getBits() const {
    if (_bits == -1) {
        int highestValue = 0;
        QMetaEnum metaEnum = getMetaEnum();
        for (int j = 0; j < metaEnum.keyCount(); j++) {
            highestValue = qMax(highestValue, metaEnum.value(j));
        }
        const_cast<EnumTypeStreamer*>(this)->_bits = getBitsForHighestValue(highestValue);
    }
    return _bits;
}

QMetaEnum EnumTypeStreamer::getMetaEnum() const {
    if (!_metaEnum.isValid()) {
        const_cast<EnumTypeStreamer*>(this)->_metaEnum = _metaObject->enumerator(_metaObject->indexOfEnumerator(_enumName));
    }
    return _metaEnum;
}

bool EnumTypeStreamer::equal(const QVariant& first, const QVariant& second) const {
    return first.toInt() == second.toInt();
}

void EnumTypeStreamer::write(Bitstream& out, const QVariant& value) const {
    int intValue = value.toInt();
    out.write(&intValue, getBits());
}

QVariant EnumTypeStreamer::read(Bitstream& in) const {
    int intValue = 0;
    in.read(&intValue, getBits());
    return intValue;
}

void EnumTypeStreamer::writeDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const {
    int intValue = value.toInt(), intReference = reference.toInt();
    if (intValue == intReference) {
        out << false;
    } else {
        out << true;
        out.write(&intValue, getBits());
    }
}

void EnumTypeStreamer::readDelta(Bitstream& in, QVariant& value, const QVariant& reference) const {
    bool changed;
    in >> changed;
    if (changed) {
        int intValue = 0;
        in.read(&intValue, getBits());
        value = intValue;
    } else {
        value = reference;
    }
}

void EnumTypeStreamer::writeRawDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const {
    int intValue = value.toInt();
    out.write(&intValue, getBits());
}

void EnumTypeStreamer::readRawDelta(Bitstream& in, QVariant& value, const QVariant& reference) const {
    int intValue = 0;
    in.read(&intValue, getBits());
    value = intValue;
}

void EnumTypeStreamer::setEnumValue(QVariant& object, int value, const QHash<int, int>& mappings) const {
    if (getMetaEnum().isFlag()) {
        int combined = 0;
        for (QHash<int, int>::const_iterator it = mappings.constBegin(); it != mappings.constEnd(); it++) {
            if (value & it.key()) {
                combined |= it.value();
            }
        }
        object = combined;
        
    } else {
        object = mappings.value(value);
    }
}

MappedEnumTypeStreamer::MappedEnumTypeStreamer(const TypeStreamer* baseStreamer, int bits, const QHash<int, int>& mappings) :
    _baseStreamer(baseStreamer),
    _bits(bits),
    _mappings(mappings) {
}

QVariant MappedEnumTypeStreamer::read(Bitstream& in) const {
    QVariant object = QVariant(_baseStreamer->getType(), 0);
    int value = 0;
    in.read(&value, _bits);
    _baseStreamer->setEnumValue(object, value, _mappings);
    return object;
}

void MappedEnumTypeStreamer::readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const {
    int value = 0;
    in.read(&value, _bits);
    _baseStreamer->setEnumValue(object, value, _mappings);
}

GenericTypeStreamer::GenericTypeStreamer(const QByteArray& name) :
    _name(name) {
}

const char* GenericTypeStreamer::getName() const {
    return _name.constData();
}

GenericEnumTypeStreamer::GenericEnumTypeStreamer(const QByteArray& name, const QVector<NameIntPair>& values,
        int bits, const QByteArray& hash) :
    GenericTypeStreamer(name),
    _values(values),
    _bits(bits),
    _hash(hash) {
    
    _type = qMetaTypeId<int>();
}

void GenericEnumTypeStreamer::writeMetadata(Bitstream& out, bool full) const {
    if (full) {
        out << _values.size();
        foreach (const NameIntPair& value, _values) {
            out << value.first << value.second;
        }
    } else {
        out << _bits;
        if (_hash.isEmpty()) {
            QCryptographicHash hash(QCryptographicHash::Md5);
            foreach (const NameIntPair& value, _values) {
                hash.addData(value.first.constData(), value.first.size() + 1);
                qint32 intValue = value.second;
                hash.addData((const char*)&intValue, sizeof(qint32));
            }
            const_cast<GenericEnumTypeStreamer*>(this)->_hash = hash.result();
        }
        out.write(_hash.constData(), _hash.size() * BITS_IN_BYTE);
    }
}

void GenericEnumTypeStreamer::write(Bitstream& out, const QVariant& value) const {
    int intValue = value.toInt();
    out.write(&intValue, _bits);
}

QVariant GenericEnumTypeStreamer::read(Bitstream& in) const {
    int intValue = 0;
    in.read(&intValue, _bits);
    return QVariant::fromValue(GenericValue(_weakSelf, intValue));
}

TypeStreamer::Category GenericEnumTypeStreamer::getCategory() const {
    return ENUM_CATEGORY;
}

MappedStreamableTypeStreamer::MappedStreamableTypeStreamer(const TypeStreamer* baseStreamer,
        const QVector<StreamerIndexPair>& fields) :
    _baseStreamer(baseStreamer),
    _fields(fields) {
}

QVariant MappedStreamableTypeStreamer::read(Bitstream& in) const {
    QVariant object = QVariant(_baseStreamer->getType(), 0);
    foreach (const StreamerIndexPair& pair, _fields) {
        QVariant value = pair.first->read(in);
        if (pair.second != -1) {
            _baseStreamer->setField(object, pair.second, value);
        }
    }
    return object;
}

void MappedStreamableTypeStreamer::readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const {
    foreach (const StreamerIndexPair& pair, _fields) {
        QVariant value;
        if (pair.second != -1) {
            pair.first->readDelta(in, value, _baseStreamer->getField(reference, pair.second));
            _baseStreamer->setField(object, pair.second, value);
        } else {
            pair.first->readDelta(in, value, QVariant());
        }
    }
}

GenericStreamableTypeStreamer::GenericStreamableTypeStreamer(const QByteArray& name,
        const QVector<StreamerNamePair>& fields, const QByteArray& hash) :
    GenericTypeStreamer(name),
    _fields(fields),
    _hash(hash) {
    
    _type = qMetaTypeId<QVariantList>();
}

void GenericStreamableTypeStreamer::writeMetadata(Bitstream& out, bool full) const {
    out << _fields.size();
    if (_fields.isEmpty()) {
        return;
    }
    foreach (const StreamerNamePair& field, _fields) {
        out << field.first.data();
        if (full) {
            out << field.second;
        }
    }
    if (!full) {
        if (_hash.isEmpty()) {
            QCryptographicHash hash(QCryptographicHash::Md5);
            foreach (const StreamerNamePair& field, _fields) {
                hash.addData(field.second.constData(), field.second.size() + 1);
            }
            const_cast<GenericStreamableTypeStreamer*>(this)->_hash = hash.result();
        }
        out.write(_hash.constData(), _hash.size() * BITS_IN_BYTE);
    }
}

void GenericStreamableTypeStreamer::write(Bitstream& out, const QVariant& value) const {
    QVariantList values = value.toList();
    for (int i = 0; i < _fields.size(); i++) {
        _fields.at(i).first->write(out, values.at(i));
    }
}

QVariant GenericStreamableTypeStreamer::read(Bitstream& in) const {
    QVariantList values;
    foreach (const StreamerNamePair& field, _fields) {
        values.append(field.first->read(in));
    }
    return QVariant::fromValue(GenericValue(_weakSelf, values));
}

TypeStreamer::Category GenericStreamableTypeStreamer::getCategory() const {
    return STREAMABLE_CATEGORY;
}

MappedListTypeStreamer::MappedListTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& valueStreamer) :
    _baseStreamer(baseStreamer),
    _valueStreamer(valueStreamer) {
}

QVariant MappedListTypeStreamer::read(Bitstream& in) const {
    QVariant object = QVariant(_baseStreamer->getType(), 0);
    int size;
    in >> size;
    for (int i = 0; i < size; i++) {
        QVariant value = _valueStreamer->read(in);
        _baseStreamer->insert(object, value);
    }
    return object;
}

void MappedListTypeStreamer::readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const {
    object = reference;
    int size, referenceSize;
    in >> size >> referenceSize;
    if (size < referenceSize) {
        _baseStreamer->prune(object, size);
    }
    for (int i = 0; i < size; i++) {
        if (i < referenceSize) {
            QVariant value;
            _valueStreamer->readDelta(in, value, _baseStreamer->getValue(reference, i));
            _baseStreamer->setValue(object, i, value);
        } else {
            _baseStreamer->insert(object, _valueStreamer->read(in));
        }
    }
}

GenericListTypeStreamer::GenericListTypeStreamer(const QByteArray& name, const TypeStreamerPointer& valueStreamer) :
    GenericTypeStreamer(name),
    _valueStreamer(valueStreamer) {
    
    _type = qMetaTypeId<QVariantList>();
}

void GenericListTypeStreamer::writeMetadata(Bitstream& out, bool full) const {
    out << _valueStreamer.data();
}

void GenericListTypeStreamer::write(Bitstream& out, const QVariant& value) const {
    QVariantList values = value.toList();
    out << values.size();
    foreach (const QVariant& element, values) {
        _valueStreamer->write(out, element);
    }
}

QVariant GenericListTypeStreamer::read(Bitstream& in) const {
    QVariantList values;
    int size;
    in >> size;
    for (int i = 0; i < size; i++) {
        values.append(_valueStreamer->read(in));
    }
    return QVariant::fromValue(GenericValue(_weakSelf, values));
}

TypeStreamer::Category GenericListTypeStreamer::getCategory() const {
    return LIST_CATEGORY;
}

MappedSetTypeStreamer::MappedSetTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& valueStreamer) :
    MappedListTypeStreamer(baseStreamer, valueStreamer) {
}

void MappedSetTypeStreamer::readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const {
    object = reference;
    int addedOrRemoved;
    in >> addedOrRemoved;
    for (int i = 0; i < addedOrRemoved; i++) {
        QVariant value = _valueStreamer->read(in);
        if (!_baseStreamer->remove(object, value)) {
            _baseStreamer->insert(object, value);
        }
    }
}

GenericSetTypeStreamer::GenericSetTypeStreamer(const QByteArray& name, const TypeStreamerPointer& valueStreamer) :
    GenericListTypeStreamer(name, valueStreamer) {
}

TypeStreamer::Category GenericSetTypeStreamer::getCategory() const {
    return SET_CATEGORY;
}

MappedMapTypeStreamer::MappedMapTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& keyStreamer,
        const TypeStreamerPointer& valueStreamer) :
    _baseStreamer(baseStreamer),
    _keyStreamer(keyStreamer),
    _valueStreamer(valueStreamer) {
}

QVariant MappedMapTypeStreamer::read(Bitstream& in) const {
    QVariant object = QVariant(_baseStreamer->getType(), 0);
    int size;
    in >> size;
    for (int i = 0; i < size; i++) {
        QVariant key = _keyStreamer->read(in);
        QVariant value = _valueStreamer->read(in);
        _baseStreamer->insert(object, key, value);
    }
    return object;
}

void MappedMapTypeStreamer::readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const {
    object = reference;
    int added;
    in >> added;
    for (int i = 0; i < added; i++) {
        QVariant key = _keyStreamer->read(in);
        QVariant value = _valueStreamer->read(in);
        _baseStreamer->insert(object, key, value);
    }
    int modified;
    in >> modified;
    for (int i = 0; i < modified; i++) {
        QVariant key = _keyStreamer->read(in);
        QVariant value;
        _valueStreamer->readDelta(in, value, _baseStreamer->getValue(reference, key));
        _baseStreamer->insert(object, key, value);
    }
    int removed;
    in >> removed;
    for (int i = 0; i < removed; i++) {
        QVariant key = _keyStreamer->read(in);
        _baseStreamer->remove(object, key);
    }
}

GenericMapTypeStreamer::GenericMapTypeStreamer(const QByteArray& name, const TypeStreamerPointer& keyStreamer,
        const TypeStreamerPointer& valueStreamer) :
    GenericTypeStreamer(name),
    _keyStreamer(keyStreamer),
    _valueStreamer(valueStreamer) {
    
    _type = qMetaTypeId<QVariantPairList>();
}

void GenericMapTypeStreamer::writeMetadata(Bitstream& out, bool full) const {
    out << _keyStreamer.data() << _valueStreamer.data();
}

void GenericMapTypeStreamer::write(Bitstream& out, const QVariant& value) const {
    QVariantPairList values = value.value<QVariantPairList>();
    out << values.size();
    foreach (const QVariantPair& pair, values) {
        _keyStreamer->write(out, pair.first);
        _valueStreamer->write(out, pair.second);
    }
}

QVariant GenericMapTypeStreamer::read(Bitstream& in) const {
    QVariantPairList values;
    int size;
    in >> size;
    for (int i = 0; i < size; i++) {
        QVariant key = _keyStreamer->read(in);
        QVariant value = _valueStreamer->read(in);
        values.append(QVariantPair(key, value));
    }
    return QVariant::fromValue(GenericValue(_weakSelf, QVariant::fromValue(values)));
}

TypeStreamer::Category GenericMapTypeStreamer::getCategory() const {
    return MAP_CATEGORY;
}

const TypeStreamer* GenericValueStreamer::getStreamerToWrite(const QVariant& value) const {
    return value.value<GenericValue>().getStreamer().data();
}
