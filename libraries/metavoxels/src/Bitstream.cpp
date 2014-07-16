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
#include <QScriptEngine>
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
REGISTER_SIMPLE_TYPE_STREAMER(QVariant)
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

void Bitstream::preThreadingInit() {
    getObjectStreamers();
    getEnumStreamers();
    getEnumStreamersByName();    
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

const ObjectStreamer* Bitstream::getObjectStreamer(const QMetaObject* metaObject) {
    return getObjectStreamers().value(metaObject);
}

const QMetaObject* Bitstream::getMetaObject(const QByteArray& className) {
    return getMetaObjects().value(className);
}

QList<const QMetaObject*> Bitstream::getMetaObjectSubClasses(const QMetaObject* metaObject) {
    return getMetaObjectSubClasses().values(metaObject);
}

QScriptValue sharedObjectPointerToScriptValue(QScriptEngine* engine, const SharedObjectPointer& pointer) {
    return pointer ? engine->newQObject(pointer.data()) : engine->nullValue();
}

void sharedObjectPointerFromScriptValue(const QScriptValue& object, SharedObjectPointer& pointer) {
    pointer = qobject_cast<SharedObject*>(object.toQObject());
}

void Bitstream::registerTypes(QScriptEngine* engine) {
    foreach (const QMetaObject* metaObject, getMetaObjects()) {
        engine->globalObject().setProperty(metaObject->className(), engine->newQMetaObject(metaObject));
    }
    qScriptRegisterMetaType(engine, sharedObjectPointerToScriptValue, sharedObjectPointerFromScriptValue);
}

Bitstream::Bitstream(QDataStream& underlying, MetadataType metadataType, GenericsMode genericsMode, QObject* parent) :
    QObject(parent),
    _underlying(underlying),
    _byte(0),
    _position(0),
    _metadataType(metadataType),
    _genericsMode(genericsMode),
    _objectStreamerStreamer(*this),
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
    WriteMappings mappings = { _objectStreamerStreamer.getAndResetTransientOffsets(),
        _typeStreamerStreamer.getAndResetTransientOffsets(),
        _attributeStreamer.getAndResetTransientOffsets(),
        _scriptStringStreamer.getAndResetTransientOffsets(),
        _sharedObjectStreamer.getAndResetTransientOffsets() };
    return mappings;
}

void Bitstream::persistWriteMappings(const WriteMappings& mappings) {
    _objectStreamerStreamer.persistTransientOffsets(mappings.objectStreamerOffsets);
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
    ReadMappings mappings = { _objectStreamerStreamer.getAndResetTransientValues(),
        _typeStreamerStreamer.getAndResetTransientValues(),
        _attributeStreamer.getAndResetTransientValues(),
        _scriptStringStreamer.getAndResetTransientValues(),
        _sharedObjectStreamer.getAndResetTransientValues() };
    return mappings;
}

void Bitstream::persistReadMappings(const ReadMappings& mappings) {
    _objectStreamerStreamer.persistTransientValues(mappings.objectStreamerValues);
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

void Bitstream::copyPersistentMappings(const Bitstream& other) {
    _objectStreamerStreamer.copyPersistentMappings(other._objectStreamerStreamer);
    _typeStreamerStreamer.copyPersistentMappings(other._typeStreamerStreamer);
    _attributeStreamer.copyPersistentMappings(other._attributeStreamer);
    _scriptStringStreamer.copyPersistentMappings(other._scriptStringStreamer);
    _sharedObjectStreamer.copyPersistentMappings(other._sharedObjectStreamer);
    _sharedObjectReferences = other._sharedObjectReferences;
    _weakSharedObjectHash = other._weakSharedObjectHash;
}

void Bitstream::clearPersistentMappings() {
    _objectStreamerStreamer.clearPersistentMappings();
    _typeStreamerStreamer.clearPersistentMappings();
    _attributeStreamer.clearPersistentMappings();
    _scriptStringStreamer.clearPersistentMappings();
    _sharedObjectStreamer.clearPersistentMappings();
    _sharedObjectReferences.clear();
    _weakSharedObjectHash.clear();
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
        _objectStreamerStreamer << NULL;
        return;
    }
    const QMetaObject* metaObject = value->metaObject();
    const ObjectStreamer* streamer = (metaObject == &GenericSharedObject::staticMetaObject) ?
        static_cast<const GenericSharedObject*>(value)->getStreamer().data() : getObjectStreamers().value(metaObject);
    _objectStreamerStreamer << streamer;
    streamer->writeRawDelta(*this, value, reference);
}

void Bitstream::readRawDelta(QObject*& value, const QObject* reference) {
    ObjectStreamerPointer streamer;
    _objectStreamerStreamer >> streamer;
    value = streamer ? streamer->readRawDelta(*this, reference) : NULL;
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
        streamer->writeVariant(*this, value);
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
        value = streamer->readVariant(*this);
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
        _objectStreamerStreamer << NULL;
        return *this;
    }
    const QMetaObject* metaObject = object->metaObject();
    const ObjectStreamer* streamer = (metaObject == &GenericSharedObject::staticMetaObject) ?
        static_cast<const GenericSharedObject*>(object)->getStreamer().data() : getObjectStreamers().value(metaObject);
    _objectStreamerStreamer << streamer;
    streamer->write(*this, object);
    return *this;
}

Bitstream& Bitstream::operator>>(QObject*& object) {
    ObjectStreamerPointer streamer;
    _objectStreamerStreamer >> streamer;
    object = streamer ? streamer->read(*this) : NULL;
    return *this;
}

Bitstream& Bitstream::operator<<(const QMetaObject* metaObject) {
    _objectStreamerStreamer << getObjectStreamers().value(metaObject);
    return *this;
}

Bitstream& Bitstream::operator>>(const QMetaObject*& metaObject) {
    ObjectStreamerPointer streamer;
    _objectStreamerStreamer >> streamer;
    metaObject = streamer->getMetaObject();
    return *this;
}

Bitstream& Bitstream::operator<<(const ObjectStreamer* streamer) {
    _objectStreamerStreamer << streamer;
    return *this;
}

Bitstream& Bitstream::operator>>(const ObjectStreamer*& streamer) {
    ObjectStreamerPointer objectStreamer;
    _objectStreamerStreamer >> objectStreamer;
    streamer = objectStreamer.data();
    return *this;
}

Bitstream& Bitstream::operator>>(ObjectStreamerPointer& streamer) {
    _objectStreamerStreamer >> streamer;
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

Bitstream& Bitstream::operator<(const ObjectStreamer* streamer) {
    if (!streamer) {
        return *this << QByteArray();
    }
    const char* name = streamer->getName();
    *this << QByteArray::fromRawData(name, strlen(name));
    if (_metadataType != NO_METADATA) {
        streamer->writeMetadata(*this, _metadataType == FULL_METADATA);
    }
    return *this;
}

Bitstream& Bitstream::operator>(ObjectStreamerPointer& streamer) {
    QByteArray className;
    *this >> className;
    if (className.isEmpty()) {
        streamer = ObjectStreamerPointer();
        return *this;
    }
    const QMetaObject* metaObject = _metaObjectSubstitutions.value(className);
    if (!metaObject) {
        metaObject = getMetaObjects().value(className);
    }
    // start out with the streamer for the named class, if any
    if (metaObject) {
        streamer = getObjectStreamers().value(metaObject)->getSelf();
    } else {
        streamer = ObjectStreamerPointer();
    }
    if (_metadataType == NO_METADATA) {
        if (!metaObject) {
            throw BitstreamException(QString("Unknown class name: ") + className);
        }
        return *this;
    }
    if (_genericsMode == ALL_GENERICS) {
        streamer = readGenericObjectStreamer(className);
        return *this;
    }
    if (!metaObject && _genericsMode == FALLBACK_GENERICS) {
        streamer = readGenericObjectStreamer(className);
        return *this;
    }
    int propertyCount;
    *this >> propertyCount;
    QVector<StreamerPropertyPair> properties(propertyCount);
    for (int i = 0; i < propertyCount; i++) {
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
        properties[i] = StreamerPropertyPair(typeStreamer, property);
    }
    // for hash metadata, check the names/types of the properties as well as the name hash against our own class
    if (_metadataType == HASH_METADATA) {
        QCryptographicHash hash(QCryptographicHash::Md5);
        bool matches = true;
        if (metaObject) {
            const QVector<StreamerPropertyPair>& localProperties = streamer->getProperties();
            if (localProperties.size() == properties.size()) {
                for (int i = 0; i < localProperties.size(); i++) {
                    const StreamerPropertyPair& localProperty = localProperties.at(i);
                    if (localProperty.first != properties.at(i).first) {
                        matches = false;
                        break;
                    }
                    hash.addData(localProperty.second.name(), strlen(localProperty.second.name()) + 1); 
                }
            } else {
                matches = false;
            }
        }
        QByteArray localHashResult = hash.result();
        QByteArray remoteHashResult(localHashResult.size(), 0);
        read(remoteHashResult.data(), remoteHashResult.size() * BITS_IN_BYTE);
        if (metaObject && matches && localHashResult == remoteHashResult) {
            return *this;
        }
    } else if (metaObject) {
        const QVector<StreamerPropertyPair>& localProperties = streamer->getProperties();
        if (localProperties.size() != properties.size()) {
            streamer = ObjectStreamerPointer(new MappedObjectStreamer(metaObject, properties));
            return *this;
        }
        for (int i = 0; i < localProperties.size(); i++) {
            const StreamerPropertyPair& property = properties.at(i);
            const StreamerPropertyPair& localProperty = localProperties.at(i);
            if (property.first != localProperty.first ||
                    property.second.propertyIndex() != localProperty.second.propertyIndex()) {
                streamer = ObjectStreamerPointer(new MappedObjectStreamer(metaObject, properties));
                return *this;
            }
        }
        return *this;
    }
    streamer = ObjectStreamerPointer(new MappedObjectStreamer(metaObject, properties));
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
            throw BitstreamException(QString("Unknown type name: ") + typeName);
        }
        return *this;
    }
    int category;
    *this >> category;
    if (category == TypeStreamer::SIMPLE_CATEGORY) {
        if (!streamer) {
            throw BitstreamException(QString("Unknown type name: ") + typeName);
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
        *this << true;
        writeRawDelta((const QObject*)object.data(), (const QObject*)reference.data());
    } else {
        *this << false;
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
    bool delta;
    *this >> delta;
    QPointer<SharedObject> reference = _sharedObjectReferences.value(originID);
    QPointer<SharedObject>& pointer = _weakSharedObjectHash[id];
    if (pointer) {
        ObjectStreamerPointer objectStreamer;
        _objectStreamerStreamer >> objectStreamer;
        if (delta) {
            if (!reference) {
                throw BitstreamException(QString("Delta without reference [id=%1, originID=%2]").arg(id).arg(originID));
            }
            objectStreamer->readRawDelta(*this, reference.data(), pointer.data());
        } else {
            objectStreamer->read(*this, pointer.data());
        }
    } else {
        QObject* rawObject; 
        if (delta) {
            if (!reference) {
                throw BitstreamException(QString("Delta without reference [id=%1, originID=%2]").arg(id).arg(originID));
            }
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

ObjectStreamerPointer Bitstream::readGenericObjectStreamer(const QByteArray& name) {
    int propertyCount;
    *this >> propertyCount;
    QVector<StreamerNamePair> properties(propertyCount);
    QByteArray hash;
    if (propertyCount > 0) {
        for (int i = 0; i < propertyCount; i++) {
            TypeStreamerPointer streamer;
            *this >> streamer;
            QByteArray name;
            if (_metadataType == FULL_METADATA) {
                *this >> name;
            }
            properties[i] = StreamerNamePair(streamer, name);
        }
        if (_metadataType == HASH_METADATA) {
            hash.resize(MD5_HASH_SIZE);
            read(hash.data(), hash.size() * BITS_IN_BYTE);
        }
    }
    ObjectStreamerPointer streamer = ObjectStreamerPointer(new GenericObjectStreamer(name, properties, hash));
    static_cast<GenericObjectStreamer*>(streamer.data())->_weakSelf = streamer;
    return streamer;
}

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

const QHash<const QMetaObject*, const ObjectStreamer*>& Bitstream::getObjectStreamers() {
    static QHash<const QMetaObject*, const ObjectStreamer*> objectStreamers = createObjectStreamers();
    return objectStreamers;
}

QHash<const QMetaObject*, const ObjectStreamer*> Bitstream::createObjectStreamers() {
    QHash<const QMetaObject*, const ObjectStreamer*> objectStreamers;
    foreach (const QMetaObject* metaObject, getMetaObjects()) {
        QVector<StreamerPropertyPair> properties;
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
                properties.append(StreamerPropertyPair(streamer->getSelf(), property));
            }
        }
        ObjectStreamerPointer streamer = ObjectStreamerPointer(new MappedObjectStreamer(metaObject, properties));
        streamer->_self = streamer;
        objectStreamers.insert(metaObject, streamer.data());
    }
    return objectStreamers;
}

QHash<int, const TypeStreamer*>& Bitstream::getTypeStreamers() {
    static QHash<int, const TypeStreamer*> typeStreamers;
    return typeStreamers;
}

const QHash<ScopeNamePair, const TypeStreamer*>& Bitstream::getEnumStreamers() {
    static QHash<ScopeNamePair, const TypeStreamer*> enumStreamers = createEnumStreamers();
    return enumStreamers;
}

static QByteArray getEnumName(const char* scope, const char* name) {
    return QByteArray(scope) + "::" + name;
}

QHash<ScopeNamePair, const TypeStreamer*> Bitstream::createEnumStreamers() {
    QHash<ScopeNamePair, const TypeStreamer*> enumStreamers;
    foreach (const QMetaObject* metaObject, getMetaObjects()) {
        for (int i = 0; i < metaObject->enumeratorCount(); i++) {
            QMetaEnum metaEnum = metaObject->enumerator(i);
            const TypeStreamer*& streamer = enumStreamers[ScopeNamePair(metaEnum.scope(), metaEnum.name())];
            if (!streamer) {
                // look for a streamer registered by name
                streamer = getTypeStreamers().value(QMetaType::type(getEnumName(metaEnum.scope(), metaEnum.name())));
                if (!streamer) {
                    streamer = new EnumTypeStreamer(metaEnum);
                }
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

BitstreamException::BitstreamException(const QString& description) :
    _description(description) {
}

QJsonValue JSONWriter::getData(bool value) {
    return value;
}

QJsonValue JSONWriter::getData(int value) {
    return value;
}

QJsonValue JSONWriter::getData(uint value) {
    return (int)value;
}

QJsonValue JSONWriter::getData(float value) {
    return (double)value;
}

QJsonValue JSONWriter::getData(const QByteArray& value) {
    return QString(value.toPercentEncoding());
}

QJsonValue JSONWriter::getData(const QColor& value) {
    return value.name();
}

QJsonValue JSONWriter::getData(const QScriptValue& value) {
    QJsonObject object;
    if (value.isUndefined()) {
        object.insert("type", QString("UNDEFINED"));
        
    } else if (value.isNull()) {
        object.insert("type", QString("NULL"));
    
    } else if (value.isBool()) {
        object.insert("type", QString("BOOL"));
        object.insert("value", value.toBool());
        
    } else if (value.isNumber()) {
        object.insert("type", QString("NUMBER"));
        object.insert("value", value.toNumber());
    
    } else if (value.isString()) {
        object.insert("type", QString("STRING"));
        object.insert("value", value.toString());
    
    } else if (value.isVariant()) {
        object.insert("type", QString("VARIANT"));
        object.insert("value", getData(value.toVariant()));
    
    } else if (value.isQObject()) {
        object.insert("type", QString("QOBJECT"));
        object.insert("value", getData(value.toQObject()));
    
    } else if (value.isQMetaObject()) {
        object.insert("type", QString("QMETAOBJECT"));
        object.insert("value", getData(value.toQMetaObject()));
        
    } else if (value.isDate()) {
        object.insert("type", QString("DATE"));
        object.insert("value", getData(value.toDateTime()));
        
    } else if (value.isRegExp()) {
        object.insert("type", QString("REGEXP"));
        object.insert("value", getData(value.toRegExp()));
    
    } else if (value.isArray()) {
        object.insert("type", QString("ARRAY"));
        QJsonArray array;
        int length = value.property(ScriptCache::getInstance()->getLengthString()).toInt32();
        for (int i = 0; i < length; i++) {
            array.append(getData(value.property(i)));
        }
        object.insert("value", array);
        
    } else if (value.isObject()) {
        object.insert("type", QString("OBJECT"));
        QJsonObject valueObject;
        for (QScriptValueIterator it(value); it.hasNext(); ) {
            it.next();
            valueObject.insert(it.name(), getData(it.value()));
        }
        object.insert("value", valueObject);
        
    } else {
        object.insert("type", QString("INVALID"));
    }
    return object;
}

QJsonValue JSONWriter::getData(const QString& value) {
    return value;
}

QJsonValue JSONWriter::getData(const QUrl& value) {
    return value.toString();
}

QJsonValue JSONWriter::getData(const QDateTime& value) {
    return (qsreal)value.toMSecsSinceEpoch();
}

QJsonValue JSONWriter::getData(const QRegExp& value) {
    QJsonObject object;
    object.insert("pattern", value.pattern());
    object.insert("caseSensitivity", (int)value.caseSensitivity());
    object.insert("patternSyntax", (int)value.patternSyntax());
    object.insert("minimal", value.isMinimal());
    return object;
}

QJsonValue JSONWriter::getData(const glm::vec3& value) {
    QJsonArray array;
    array.append(value.x);
    array.append(value.y);
    array.append(value.z);
    return array;
}

QJsonValue JSONWriter::getData(const glm::quat& value) {
    QJsonArray array;
    array.append(value.x);
    array.append(value.y);
    array.append(value.z);
    array.append(value.w);
    return array;
}

QJsonValue JSONWriter::getData(const QMetaObject* metaObject) {
    if (!metaObject) {
        return QJsonValue();
    }
    const ObjectStreamer* streamer = Bitstream::getObjectStreamers().value(metaObject);
    addObjectStreamer(streamer);
    return QString(streamer->getName());
}

QJsonValue JSONWriter::getData(const QVariant& value) {
    if (!value.isValid()) {
        return QJsonValue();
    }
    const TypeStreamer* streamer = Bitstream::getTypeStreamers().value(value.userType());
    if (streamer) {
        return streamer->getJSONVariantData(*this, value);
    } else {
        qWarning() << "Non-streamable type:" << value.typeName();
        return QJsonValue();
    }
}

QJsonValue JSONWriter::getData(const SharedObjectPointer& object) {
    if (object) {
        addSharedObject(object);
        return object->getID();
    } else {
        return 0;
    }
}

QJsonValue JSONWriter::getData(const QObject* object) {
    if (!object) {
        return QJsonValue();
    }
    const QMetaObject* metaObject = object->metaObject();
    const ObjectStreamer* streamer = (metaObject == &GenericSharedObject::staticMetaObject) ?
        static_cast<const GenericSharedObject*>(object)->getStreamer().data() :
            Bitstream::getObjectStreamers().value(metaObject);
    return streamer->getJSONData(*this, object);
}

QJsonValue JSONWriter::getData(const GenericValue& value) {
    return value.getStreamer()->getJSONData(*this, value.getValue());
}

void JSONWriter::addTypeStreamer(const TypeStreamer* streamer) {
    if (!_typeStreamerNames.contains(streamer->getName())) {
        _typeStreamerNames.insert(streamer->getName());
        
        QJsonValue metadata = streamer->getJSONMetadata(*this);
        if (!metadata.isNull()) {
            _typeStreamers.append(metadata);
        }
    }
}

void JSONWriter::addObjectStreamer(const ObjectStreamer* streamer) {
    if (!_objectStreamerNames.contains(streamer->getName())) {
        _objectStreamerNames.insert(streamer->getName());
        _objectStreamers.append(streamer->getJSONMetadata(*this));
    }
}

void JSONWriter::addSharedObject(const SharedObjectPointer& object) {
    if (!_sharedObjectIDs.contains(object->getID())) {
        _sharedObjectIDs.insert(object->getID());
        
        QJsonObject sharedObject;
        sharedObject.insert("id", object->getID());
        sharedObject.insert("data", getData(static_cast<const QObject*>(object.data())));
        _sharedObjects.append(sharedObject);
    }
}

QJsonDocument JSONWriter::getDocument() const {
    QJsonObject top;
    top.insert("contents", _contents);
    top.insert("objects", _sharedObjects);
    top.insert("classes", _objectStreamers);
    top.insert("types", _typeStreamers);
    return QJsonDocument(top);
}

JSONReader::JSONReader(const QJsonDocument& document, Bitstream::GenericsMode genericsMode) {
    // create and map the type streamers in order
    QJsonObject top = document.object();
    foreach (const QJsonValue& element, top.value("types").toArray()) {
        QJsonObject type = element.toObject();
        QString name = type.value("name").toString();
        QByteArray latinName = name.toLatin1();
        const TypeStreamer* baseStreamer = Bitstream::getTypeStreamers().value(QMetaType::type(latinName));
        if (!baseStreamer) {
            baseStreamer = Bitstream::getEnumStreamersByName().value(latinName);
        }
        if (!baseStreamer && genericsMode == Bitstream::NO_GENERICS) {
            continue; // no built-in type and no generics allowed; we give up
        }
        QString category = type.value("category").toString();
        if (!baseStreamer || genericsMode == Bitstream::ALL_GENERICS) {
            // create a generic streamer
            TypeStreamerPointer streamer;
            if (category == "ENUM") {
                QVector<NameIntPair> values;
                int highestValue = 0;
                foreach (const QJsonValue& value, type.value("values").toArray()) {
                    QJsonObject object = value.toObject();
                    int intValue = object.value("value").toInt();
                    highestValue = qMax(intValue, highestValue);
                    values.append(NameIntPair(object.value("key").toString().toLatin1(), intValue));
                }
                streamer = TypeStreamerPointer(new GenericEnumTypeStreamer(latinName,
                    values, getBitsForHighestValue(highestValue), QByteArray()));
            
            } else if (category == "STREAMABLE") {
                QVector<StreamerNamePair> fields;
                foreach (const QJsonValue& field, type.value("fields").toArray()) {
                    QJsonObject object = field.toObject();
                    fields.append(StreamerNamePair(getTypeStreamer(object.value("type").toString()),
                        object.value("name").toString().toLatin1()));
                }
                streamer = TypeStreamerPointer(new GenericStreamableTypeStreamer(latinName,
                    fields, QByteArray()));
            
            } else if (category == "LIST") {
                streamer = TypeStreamerPointer(new GenericListTypeStreamer(latinName,
                    getTypeStreamer(type.value("valueType").toString())));
            
            } else if (category == "SET") {
                streamer = TypeStreamerPointer(new GenericSetTypeStreamer(latinName,
                    getTypeStreamer(type.value("valueType").toString())));
                    
            } else if (category == "MAP") {
                streamer = TypeStreamerPointer(new GenericMapTypeStreamer(latinName,
                    getTypeStreamer(type.value("keyType").toString()),
                    getTypeStreamer(type.value("valueType").toString())));
            }
            _typeStreamers.insert(name, streamer);
            static_cast<GenericTypeStreamer*>(streamer.data())->_weakSelf = streamer;
            continue;
        }
        // create a mapped streamer, determining along the way whether it matches our base
        if (category == "ENUM") {
            QHash<int, int> mappings;
            int highestValue = 0;
            QMetaEnum metaEnum = baseStreamer->getMetaEnum();
            QJsonArray array = type.value("values").toArray();
            bool matches = (array.size() == metaEnum.keyCount());
            foreach (const QJsonValue& value, array) {
                QJsonObject object = value.toObject();
                int intValue = object.value("value").toInt();
                highestValue = qMax(intValue, highestValue);
                int mapping = metaEnum.keyToValue(object.value("key").toString().toLatin1());
                if (mapping != -1) {
                    mappings.insert(intValue, mapping);
                }
                matches &= (intValue == mapping);
            }
            // if everything matches our built-in enum, we can use that, which will be faster
            if (matches) {
                _typeStreamers.insert(name, baseStreamer->getSelf());
            } else {
                _typeStreamers.insert(name, TypeStreamerPointer(new MappedEnumTypeStreamer(baseStreamer,
                    getBitsForHighestValue(highestValue), mappings)));
            }
        } else if (category == "STREAMABLE") {
            QVector<StreamerIndexPair> fields;
            QJsonArray array = type.value("fields").toArray();
            const QVector<MetaField>& metaFields = baseStreamer->getMetaFields(); 
            bool matches = (array.size() == metaFields.size());
            for (int i = 0; i < array.size(); i++) {
                QJsonObject object = array.at(i).toObject();
                TypeStreamerPointer streamer = getTypeStreamer(object.value("type").toString());
                int index = baseStreamer->getFieldIndex(object.value("name").toString().toLatin1());
                fields.append(StreamerIndexPair(streamer, index));
                matches &= (index == i && streamer == metaFields.at(i).getStreamer());
            }
            // if everything matches our built-in streamable, we can use that, which will be faster
            if (matches) {
                _typeStreamers.insert(name, baseStreamer->getSelf());
            } else {
                _typeStreamers.insert(name, TypeStreamerPointer(new MappedStreamableTypeStreamer(baseStreamer, fields)));
            }
        } else if (category == "LIST") {
            TypeStreamerPointer valueStreamer = getTypeStreamer(type.value("valueType").toString());
            if (valueStreamer == baseStreamer->getValueStreamer()) {
                _typeStreamers.insert(name, baseStreamer->getSelf());
                            
            } else {
                _typeStreamers.insert(name, TypeStreamerPointer(new MappedListTypeStreamer(baseStreamer, valueStreamer)));
            }
        } else if (category == "SET") {
            TypeStreamerPointer valueStreamer = getTypeStreamer(type.value("valueType").toString());
            if (valueStreamer == baseStreamer->getValueStreamer()) {
                _typeStreamers.insert(name, baseStreamer->getSelf());
                            
            } else {
                _typeStreamers.insert(name, TypeStreamerPointer(new MappedSetTypeStreamer(baseStreamer, valueStreamer)));
            }
        } else if (category == "MAP") {
            TypeStreamerPointer keyStreamer = getTypeStreamer(type.value("keyType").toString());
            TypeStreamerPointer valueStreamer = getTypeStreamer(type.value("valueType").toString());
            if (keyStreamer == baseStreamer->getKeyStreamer() && valueStreamer == baseStreamer->getValueStreamer()) {
                _typeStreamers.insert(name, baseStreamer->getSelf());   
                
            } else {
                _typeStreamers.insert(name, TypeStreamerPointer(new MappedMapTypeStreamer(
                    baseStreamer, keyStreamer, valueStreamer)));
            }
        }
    }

    // create and map the object streamers in order
    foreach (const QJsonValue& element, top.value("classes").toArray()) {
        QJsonObject clazz = element.toObject();
        QString name = clazz.value("name").toString();
        QByteArray latinName = name.toLatin1();
        const ObjectStreamer* baseStreamer = Bitstream::getObjectStreamers().value(
            Bitstream::getMetaObjects().value(latinName));
        if (!baseStreamer && genericsMode == Bitstream::NO_GENERICS) {
            continue; // no built-in class and no generics allowed; we give up
        }
        if (!baseStreamer || genericsMode == Bitstream::ALL_GENERICS) {
            // create a generic streamer
            QVector<StreamerNamePair> properties;
            foreach (const QJsonValue& property, clazz.value("properties").toArray()) {
                QJsonObject object = property.toObject();
                properties.append(StreamerNamePair(getTypeStreamer(object.value("type").toString()),
                    object.value("name").toString().toLatin1()));
            }
            ObjectStreamerPointer streamer = ObjectStreamerPointer(new GenericObjectStreamer(
                latinName, properties, QByteArray()));
            _objectStreamers.insert(name, streamer);
            static_cast<GenericObjectStreamer*>(streamer.data())->_weakSelf = streamer;
            continue;
        }
        // create a mapped streamer, determining along the way whether it matches our base
        const QMetaObject* metaObject = baseStreamer->getMetaObject();
        const QVector<StreamerPropertyPair>& baseProperties = baseStreamer->getProperties();
        QVector<StreamerPropertyPair> properties;
        QJsonArray propertyArray = clazz.value("properties").toArray();
        bool matches = (baseProperties.size() == propertyArray.size());
        for (int i = 0; i < propertyArray.size(); i++) {
            QJsonObject object = propertyArray.at(i).toObject();
            TypeStreamerPointer typeStreamer = getTypeStreamer(object.value("type").toString());
            QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(
                object.value("name").toString().toLatin1()));
            properties.append(StreamerPropertyPair(typeStreamer, metaProperty));
            
            const StreamerPropertyPair& baseProperty = baseProperties.at(i);
            matches &= (typeStreamer == baseProperty.first &&
                metaProperty.propertyIndex() == baseProperty.second.propertyIndex());
        }
        // if everything matches our built-in type, we can use that directly, which will be faster
        if (matches) {
            _objectStreamers.insert(name, baseStreamer->getSelf());
        } else {
            _objectStreamers.insert(name, ObjectStreamerPointer(new MappedObjectStreamer(metaObject, properties)));
        }
    }
    
    // create and map the objects in order
    foreach (const QJsonValue& element, top.value("objects").toArray()) {
        QJsonObject object = element.toObject();
        int id = object.value("id").toInt();
        QObject* qObject;
        putData(object.value("data"), qObject);
        if (qObject) {
            _sharedObjects.insert(id, static_cast<SharedObject*>(qObject));
        }
    }
    
    // prepare the contents for extraction
    _contents = top.value("contents").toArray();
    _contentsIterator = _contents.constBegin();
}

void JSONReader::putData(const QJsonValue& data, bool& value) {
    value = data.toBool();
}

void JSONReader::putData(const QJsonValue& data, int& value) {
    value = data.toInt();
}

void JSONReader::putData(const QJsonValue& data, uint& value) {
    value = data.toInt();
}

void JSONReader::putData(const QJsonValue& data, float& value) {
    value = data.toDouble();
}

void JSONReader::putData(const QJsonValue& data, QByteArray& value) {
    value = QByteArray::fromPercentEncoding(data.toString().toLatin1());
}

void JSONReader::putData(const QJsonValue& data, QColor& value) {
    value.setNamedColor(data.toString());
}

void JSONReader::putData(const QJsonValue& data, QScriptValue& value) {
    QJsonObject object = data.toObject();
    QString type = object.value("type").toString();
    if (type == "UNDEFINED") {
        value = QScriptValue(QScriptValue::UndefinedValue);
        
    } else if (type == "NULL") {
        value = QScriptValue(QScriptValue::NullValue);
    
    } else if (type == "BOOL") {
        value = QScriptValue(object.value("value").toBool());
    
    } else if (type == "NUMBER") {
        value = QScriptValue(object.value("value").toDouble());
    
    } else if (type == "STRING") {
        value = QScriptValue(object.value("value").toString());
    
    } else if (type == "VARIANT") {
        QVariant variant;
        putData(object.value("value"), variant);
        value = ScriptCache::getInstance()->getEngine()->newVariant(variant);
        
    } else if (type == "QOBJECT") {
        QObject* qObject;
        putData(object.value("value"), qObject);
        value = ScriptCache::getInstance()->getEngine()->newQObject(qObject, QScriptEngine::ScriptOwnership);
    
    } else if (type == "QMETAOBJECT") {
        const QMetaObject* metaObject;
        putData(object.value("value"), metaObject);
        value = ScriptCache::getInstance()->getEngine()->newQMetaObject(metaObject);
    
    } else if (type == "DATE") {
        QDateTime dateTime;
        putData(object.value("value"), dateTime);
        value = ScriptCache::getInstance()->getEngine()->newDate(dateTime);
    
    } else if (type == "REGEXP") {
        QRegExp regExp;
        putData(object.value("value"), regExp);
        value = ScriptCache::getInstance()->getEngine()->newRegExp(regExp);
    
    } else if (type == "ARRAY") {
        QJsonArray array = object.value("value").toArray();
        value = ScriptCache::getInstance()->getEngine()->newArray(array.size());
        for (int i = 0; i < array.size(); i++) {
            QScriptValue element;
            putData(array.at(i), element);
            value.setProperty(i, element);
        }
    } else if (type == "OBJECT") {
        QJsonObject jsonObject = object.value("value").toObject();
        value = ScriptCache::getInstance()->getEngine()->newObject();
        for (QJsonObject::const_iterator it = jsonObject.constBegin(); it != jsonObject.constEnd(); it++) {
            QScriptValue element;
            putData(it.value(), element);
            value.setProperty(it.key(), element); 
        }
    } else {
        value = QScriptValue();
    }
}

void JSONReader::putData(const QJsonValue& data, QString& value) {
    value = data.toString();
}

void JSONReader::putData(const QJsonValue& data, QUrl& value) {
    value = data.toString();
}

void JSONReader::putData(const QJsonValue& data, QDateTime& value) {
    value.setMSecsSinceEpoch((qint64)data.toDouble());
}

void JSONReader::putData(const QJsonValue& data, QRegExp& value) {
    QJsonObject object = data.toObject();
    value = QRegExp(object.value("pattern").toString(), (Qt::CaseSensitivity)object.value("caseSensitivity").toInt(),
        (QRegExp::PatternSyntax)object.value("patternSyntax").toInt());
    value.setMinimal(object.value("minimal").toBool());
}

void JSONReader::putData(const QJsonValue& data, glm::vec3& value) {
    QJsonArray array = data.toArray();
    value = glm::vec3(array.at(0).toDouble(), array.at(1).toDouble(), array.at(2).toDouble());
}

void JSONReader::putData(const QJsonValue& data, glm::quat& value) {
    QJsonArray array = data.toArray();
    value = glm::quat(array.at(0).toDouble(), array.at(1).toDouble(), array.at(2).toDouble(), array.at(3).toDouble());
}

void JSONReader::putData(const QJsonValue& data, const QMetaObject*& value) {
    ObjectStreamerPointer streamer = _objectStreamers.value(data.toString());
    value = streamer ? streamer->getMetaObject() : NULL;
}

void JSONReader::putData(const QJsonValue& data, QVariant& value) {
    QJsonObject object = data.toObject();
    QString type = object.value("type").toString();
    TypeStreamerPointer streamer = getTypeStreamer(type);
    if (streamer) {
        streamer->putJSONVariantData(*this, object.value("value"), value);
    } else {
        value = QVariant();
    }
}

void JSONReader::putData(const QJsonValue& data, SharedObjectPointer& value) {
    value = _sharedObjects.value(data.toInt()); 
}

void JSONReader::putData(const QJsonValue& data, QObject*& value) {
    QJsonObject object = data.toObject();
    ObjectStreamerPointer streamer = _objectStreamers.value(object.value("class").toString());
    value = streamer ? streamer->putJSONData(*this, object) : NULL;
}

TypeStreamerPointer JSONReader::getTypeStreamer(const QString& name) const {
    TypeStreamerPointer streamer = _typeStreamers.value(name);
    if (!streamer) {
        const TypeStreamer* defaultStreamer = Bitstream::getTypeStreamers().value(QMetaType::type(name.toLatin1()));
        if (defaultStreamer) {
            streamer = defaultStreamer->getSelf();
        } else {
            qWarning() << "Unknown type:" << name;
        }
    }
    return streamer;
}

ObjectStreamer::ObjectStreamer(const QMetaObject* metaObject) :
    _metaObject(metaObject) {
}

ObjectStreamer::~ObjectStreamer() {
}

const QVector<StreamerPropertyPair>& ObjectStreamer::getProperties() const {
    static QVector<StreamerPropertyPair> emptyProperties;
    return emptyProperties;
}

MappedObjectStreamer::MappedObjectStreamer(const QMetaObject* metaObject, const QVector<StreamerPropertyPair>& properties) :
    ObjectStreamer(metaObject),
    _properties(properties) {
}

const char* MappedObjectStreamer::getName() const {
    return _metaObject->className();
}

const QVector<StreamerPropertyPair>& MappedObjectStreamer::getProperties() const {
    return _properties;
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

QJsonObject MappedObjectStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(_metaObject->className()));
    QJsonArray properties;
    foreach (const StreamerPropertyPair& property, _properties) {
        QJsonObject object;
        writer.addTypeStreamer(property.first.data());
        object.insert("type", QString(property.first->getName()));
        object.insert("name", QString(property.second.name()));
        properties.append(object);
    }
    metadata.insert("properties", properties);
    return metadata;
}

QJsonObject MappedObjectStreamer::getJSONData(JSONWriter& writer, const QObject* object) const {
    QJsonObject data;
    writer.addObjectStreamer(this);
    data.insert("class", QString(_metaObject->className()));
    QJsonArray properties;
    foreach (const StreamerPropertyPair& property, _properties) {
        properties.append(property.first->getJSONData(writer, property.second.read(object)));
    }
    data.insert("properties", properties);
    return data;
}

QObject* MappedObjectStreamer::putJSONData(JSONReader& reader, const QJsonObject& jsonObject) const {
    if (!_metaObject) {
        return NULL;
    }
    QObject* object = _metaObject->newInstance();
    QJsonArray properties = jsonObject.value("properties").toArray();
    for (int i = 0; i < _properties.size(); i++) {
        const StreamerPropertyPair& property = _properties.at(i);
        if (property.second.isValid()) {
            QVariant value;
            property.first->putJSONData(reader, properties.at(i), value);
            property.second.write(object, value);
        }
    }
    return object;
}

bool MappedObjectStreamer::equal(const QObject* first, const QObject* second) const {
    foreach (const StreamerPropertyPair& property, _properties) {
        if (!property.first->equal(property.second.read(first), property.second.read(second))) {
            return false;
        }
    }
    return true;
}

void MappedObjectStreamer::write(Bitstream& out, const QObject* object) const {
    foreach (const StreamerPropertyPair& property, _properties) {
        property.first->write(out, property.second.read(object));
    }
}

void MappedObjectStreamer::writeRawDelta(Bitstream& out, const QObject* object, const QObject* reference) const {
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

QObject* MappedObjectStreamer::readRawDelta(Bitstream& in, const QObject* reference, QObject* object) const {
    if (!object && _metaObject) {
        object = _metaObject->newInstance();
    }
    foreach (const StreamerPropertyPair& property, _properties) {
        QVariant value;
        property.first->readDelta(in, value, (property.second.isValid() && reference &&
            reference->metaObject() == _metaObject) ? property.second.read(reference) : QVariant());
        if (property.second.isValid() && object) {
            property.second.write(object, value);
        }
    }
    return object;
}

GenericObjectStreamer::GenericObjectStreamer(const QByteArray& name, const QVector<StreamerNamePair>& properties,
        const QByteArray& hash) :
    ObjectStreamer(&GenericSharedObject::staticMetaObject),
    _name(name),
    _properties(properties),
    _hash(hash) {
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

QJsonObject GenericObjectStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(_name));
    QJsonArray properties;
    foreach (const StreamerNamePair& property, _properties) {
        QJsonObject object;
        writer.addTypeStreamer(property.first.data());
        object.insert("type", QString(property.first->getName()));
        object.insert("name", QString(property.second));
        properties.append(object);
    }
    metadata.insert("properties", properties);
    return metadata;
}

QJsonObject GenericObjectStreamer::getJSONData(JSONWriter& writer, const QObject* object) const {
    QJsonObject data;
    writer.addObjectStreamer(this);
    data.insert("class", QString(_name));
    QJsonArray properties;
    const QVariantList& values = static_cast<const GenericSharedObject*>(object)->getValues();
    for (int i = 0; i < _properties.size(); i++) {
        properties.append(_properties.at(i).first->getJSONData(writer, values.at(i)));
    }
    data.insert("properties", properties);
    return data;
}

QObject* GenericObjectStreamer::putJSONData(JSONReader& reader, const QJsonObject& jsonObject) const {
    GenericSharedObject* object = new GenericSharedObject(_weakSelf);
    QJsonArray properties = jsonObject.value("properties").toArray();
    QVariantList values;
    for (int i = 0; i < _properties.size(); i++) {    
        QVariant value;
        _properties.at(i).first->putJSONData(reader, properties.at(i), value);
        values.append(value);
    }
    object->setValues(values);
    return object;
}

bool GenericObjectStreamer::equal(const QObject* first, const QObject* second) const {
    const QVariantList& firstValues = static_cast<const GenericSharedObject*>(first)->getValues();
    const QVariantList& secondValues = static_cast<const GenericSharedObject*>(second)->getValues();
    for (int i = 0; i < _properties.size(); i++) {
        if (!_properties.at(i).first->equal(firstValues.at(i), secondValues.at(i))) {
            return false;
        }
    }
    return true;
}

void GenericObjectStreamer::write(Bitstream& out, const QObject* object) const {
    const QVariantList& values = static_cast<const GenericSharedObject*>(object)->getValues();
    for (int i = 0; i < _properties.size(); i++) {
        _properties.at(i).first->write(out, values.at(i));
    }
}

void GenericObjectStreamer::writeRawDelta(Bitstream& out, const QObject* object, const QObject* reference) const {
    const GenericSharedObject* genericObject = static_cast<const GenericSharedObject*>(object);
    const GenericSharedObject* genericReference = (reference &&
        reference->metaObject() == &GenericSharedObject::staticMetaObject) ?
            static_cast<const GenericSharedObject*>(reference) : NULL;
    for (int i = 0; i < _properties.size(); i++) {
        _properties.at(i).first->writeDelta(out, genericObject->getValues().at(i),
            (genericReference && genericReference->getStreamer() == genericObject->getStreamer()) ?
                genericReference->getValues().at(i) : QVariant());
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

QObject* GenericObjectStreamer::readRawDelta(Bitstream& in, const QObject* reference, QObject* object) const {
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

QJsonValue TypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    Category category = getCategory();
    switch (category) {
        case STREAMABLE_CATEGORY: {
            QJsonObject metadata;
            metadata.insert("name", QString(getName()));
            metadata.insert("category", QString("STREAMABLE"));
            QJsonArray fields;
            foreach (const MetaField& metaField, getMetaFields()) {
                QJsonObject field;
                writer.addTypeStreamer(metaField.getStreamer());
                field.insert("type", QString(metaField.getStreamer()->getName()));
                field.insert("name", QString(metaField.getName()));
                fields.append(field);
            }
            metadata.insert("fields", fields);
            return metadata;
        }
        case LIST_CATEGORY:
        case SET_CATEGORY: {
            QJsonObject metadata;
            metadata.insert("name", QString(getName()));
            metadata.insert("category", QString(category == LIST_CATEGORY ? "LIST" : "SET"));
            const TypeStreamer* valueStreamer = getValueStreamer();
            writer.addTypeStreamer(valueStreamer);
            metadata.insert("valueType", QString(valueStreamer->getName()));
            return metadata;
        }
        case MAP_CATEGORY: {
            QJsonObject metadata;
            metadata.insert("name", QString(getName()));
            metadata.insert("category", QString("MAP"));
            const TypeStreamer* keyStreamer = getKeyStreamer();
            writer.addTypeStreamer(keyStreamer);
            metadata.insert("keyType", QString(keyStreamer->getName()));
            const TypeStreamer* valueStreamer = getValueStreamer();
            writer.addTypeStreamer(valueStreamer);
            metadata.insert("valueType", QString(valueStreamer->getName()));
            return metadata;
        }
        default:
            return QJsonValue();
    }
}

QJsonValue TypeStreamer::getJSONData(JSONWriter& writer, const QVariant& value) const {
    return QJsonValue();
}

QJsonValue TypeStreamer::getJSONVariantData(JSONWriter& writer, const QVariant& value) const {
    writer.addTypeStreamer(this);
    QJsonObject data;
    data.insert("type", QString(getName()));
    data.insert("value", getJSONData(writer, value));
    return data;   
}

void TypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = QVariant();
}

void TypeStreamer::putJSONVariantData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    putJSONData(reader, data, value);
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

void TypeStreamer::writeVariant(Bitstream& out, const QVariant& value) const {
    out << this;
    write(out, value);
}

QVariant TypeStreamer::readVariant(Bitstream& in) const {
    return read(in);
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
    _name(getEnumName(metaObject->className(), name)),
    _bits(-1) {
    
    _type = QMetaType::Int;
    _self = TypeStreamerPointer(this);
}

EnumTypeStreamer::EnumTypeStreamer(const QMetaEnum& metaEnum) :
    _name(getEnumName(metaEnum.scope(), metaEnum.name())),
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

QJsonValue EnumTypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(getName()));
    metadata.insert("category", QString("ENUM"));
    QJsonArray values;
    QMetaEnum metaEnum = getMetaEnum();
    for (int i = 0; i < metaEnum.keyCount(); i++) {
        QJsonObject value;
        value.insert("key", QString(metaEnum.key(i)));
        value.insert("value", metaEnum.value(i));
        values.append(value);
    }
    metadata.insert("values", values);
    return metadata;
}

QJsonValue EnumTypeStreamer::getJSONData(JSONWriter& writer, const QVariant& value) const {
    return value.toInt();
}

void EnumTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = data.toInt();
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

void MappedEnumTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = QVariant(_baseStreamer->getType(), 0);
    _baseStreamer->setEnumValue(value, data.toInt(), _mappings);
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

void GenericTypeStreamer::putJSONVariantData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    QVariant containedValue;
    putJSONData(reader, data, containedValue);
    value = QVariant::fromValue(GenericValue(_weakSelf, containedValue));
}

QVariant GenericTypeStreamer::readVariant(Bitstream& in) const {
    return QVariant::fromValue(GenericValue(_weakSelf, read(in)));
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

QJsonValue GenericEnumTypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(getName()));
    metadata.insert("category", QString("ENUM"));
    QJsonArray values;
    foreach (const NameIntPair& value, _values) {
        QJsonObject object;
        object.insert("key", QString(value.first));
        object.insert("value", value.second);
        values.append(object);
    }
    metadata.insert("values", values);
    return metadata;
}

QJsonValue GenericEnumTypeStreamer::getJSONData(JSONWriter& writer, const QVariant& value) const {
    return value.toInt();
}

void GenericEnumTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = data.toInt();
}

void GenericEnumTypeStreamer::write(Bitstream& out, const QVariant& value) const {
    int intValue = value.toInt();
    out.write(&intValue, _bits);
}

QVariant GenericEnumTypeStreamer::read(Bitstream& in) const {
    int intValue = 0;
    in.read(&intValue, _bits);
    return intValue;
}

TypeStreamer::Category GenericEnumTypeStreamer::getCategory() const {
    return ENUM_CATEGORY;
}

MappedStreamableTypeStreamer::MappedStreamableTypeStreamer(const TypeStreamer* baseStreamer,
        const QVector<StreamerIndexPair>& fields) :
    _baseStreamer(baseStreamer),
    _fields(fields) {
}

void MappedStreamableTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = QVariant(_baseStreamer->getType(), 0);
    QJsonArray array = data.toArray();
    for (int i = 0; i < _fields.size(); i++) {
        const StreamerIndexPair& pair = _fields.at(i);
        if (pair.second != -1) {
            QVariant element;
            pair.first->putJSONData(reader, array.at(i), element);
            _baseStreamer->setField(value, pair.second, element);    
        }
    }
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

QJsonValue GenericStreamableTypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(getName()));
    metadata.insert("category", QString("STREAMABLE"));
    QJsonArray fields;
    foreach (const StreamerNamePair& field, _fields) {
        QJsonObject object;
        writer.addTypeStreamer(field.first.data());
        object.insert("type", QString(field.first->getName()));
        object.insert("name", QString(field.second));
        fields.append(object);
    }
    metadata.insert("fields", fields);
    return metadata;
}

QJsonValue GenericStreamableTypeStreamer::getJSONData(JSONWriter& writer, const QVariant& value) const {
    QVariantList values = value.toList();
    QJsonArray array;
    for (int i = 0; i < _fields.size(); i++) {
        array.append(_fields.at(i).first->getJSONData(writer, values.at(i)));
    }
    return array;
}

void GenericStreamableTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    QJsonArray array = data.toArray();
    QVariantList values;
    for (int i = 0; i < _fields.size(); i++) {
        QVariant element;
        _fields.at(i).first->putJSONData(reader, array.at(i), element);
        values.append(element);
    }
    value = values;
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
    return values;
}

TypeStreamer::Category GenericStreamableTypeStreamer::getCategory() const {
    return STREAMABLE_CATEGORY;
}

MappedListTypeStreamer::MappedListTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& valueStreamer) :
    _baseStreamer(baseStreamer),
    _valueStreamer(valueStreamer) {
}

void MappedListTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = QVariant(_baseStreamer->getType(), 0);
    foreach (const QJsonValue& element, data.toArray()) {
        QVariant elementValue;
        _valueStreamer->putJSONData(reader, element, elementValue);
        _baseStreamer->insert(value, elementValue);
    }
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

QJsonValue GenericListTypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(getName()));
    metadata.insert("category", QString("LIST"));
    writer.addTypeStreamer(_valueStreamer.data());
    metadata.insert("valueType", QString(_valueStreamer->getName()));
    return metadata;
}

QJsonValue GenericListTypeStreamer::getJSONData(JSONWriter& writer, const QVariant& value) const {
    QVariantList values = value.toList();
    QJsonArray array;
    foreach (const QVariant& element, values) {
        array.append(_valueStreamer->getJSONData(writer, element));
    }
    return array;
}

void GenericListTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    QJsonArray array = data.toArray();
    QVariantList values;
    foreach (const QJsonValue& element, array) {
        QVariant elementValue;
        _valueStreamer->putJSONData(reader, element, elementValue);
        values.append(elementValue);
    }
    value = values;
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
    return values;
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

QJsonValue GenericSetTypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(getName()));
    metadata.insert("category", QString("SET"));
    writer.addTypeStreamer(_valueStreamer.data());
    metadata.insert("valueType", QString(_valueStreamer->getName()));
    return metadata;
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

void MappedMapTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    value = QVariant(_baseStreamer->getType(), 0);
    foreach (const QJsonValue& element, data.toArray()) {
        QJsonArray pair = element.toArray();
        QVariant elementKey;
        _keyStreamer->putJSONData(reader, pair.at(0), elementKey);
        QVariant elementValue;
        _valueStreamer->putJSONData(reader, pair.at(1), elementValue);
        _baseStreamer->insert(value, elementKey, elementValue);
    }
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

QJsonValue GenericMapTypeStreamer::getJSONMetadata(JSONWriter& writer) const {
    QJsonObject metadata;
    metadata.insert("name", QString(getName()));
    metadata.insert("category", QString("MAP"));
    writer.addTypeStreamer(_keyStreamer.data());
    metadata.insert("keyType", QString(_keyStreamer->getName()));
    writer.addTypeStreamer(_valueStreamer.data());
    metadata.insert("valueType", QString(_valueStreamer->getName()));
    return metadata;
}

QJsonValue GenericMapTypeStreamer::getJSONData(JSONWriter& writer, const QVariant& value) const {
    QVariantPairList values = value.value<QVariantPairList>();
    QJsonArray array;
    foreach (const QVariantPair& pair, values) {
        QJsonArray pairArray;
        pairArray.append(_keyStreamer->getJSONData(writer, pair.first));
        pairArray.append(_valueStreamer->getJSONData(writer, pair.second));
        array.append(pairArray);
    }
    return array;
}

void GenericMapTypeStreamer::putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
    QJsonArray array = data.toArray();
    QVariantPairList values;
    foreach (const QJsonValue& element, array) {
        QJsonArray pair = element.toArray();
        QVariant elementKey;
        _keyStreamer->putJSONData(reader, pair.at(0), elementKey);
        QVariant elementValue;
        _valueStreamer->putJSONData(reader, pair.at(1), elementValue);
        values.append(QVariantPair(elementKey, elementValue));
    }
    value = QVariant::fromValue(values);
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
    return QVariant::fromValue(values);
}

TypeStreamer::Category GenericMapTypeStreamer::getCategory() const {
    return MAP_CATEGORY;
}

QJsonValue GenericValueStreamer::getJSONVariantData(JSONWriter& writer, const QVariant& value) const {
    GenericValue genericValue = value.value<GenericValue>();
    writer.addTypeStreamer(genericValue.getStreamer().data());
    QJsonObject data;
    data.insert("type", QString(genericValue.getStreamer()->getName()));
    data.insert("value", genericValue.getStreamer()->getJSONData(writer, genericValue.getValue()));
    return data;
}

void GenericValueStreamer::writeVariant(Bitstream& out, const QVariant& value) const {
    GenericValue genericValue = value.value<GenericValue>();
    out << genericValue.getStreamer().data();
    genericValue.getStreamer()->write(out, genericValue.getValue());
}

