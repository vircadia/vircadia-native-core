//
//  Bitstream.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Bitstream_h
#define hifi_Bitstream_h

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QMetaType>
#include <QPointer>
#include <QScriptString>
#include <QSharedPointer>
#include <QVariant>
#include <QVector>
#include <QtDebug>

#include <glm/glm.hpp>

#include "SharedObject.h"

class QByteArray;
class QColor;
class QDataStream;
class QScriptEngine;
class QScriptValue;
class QUrl;

class Attribute;
class AttributeValue;
class Bitstream;
class GenericValue;
class JSONWriter;
class ObjectReader;
class ObjectStreamer;
class OwnedAttributeValue;
class TypeStreamer;

typedef SharedObjectPointerTemplate<Attribute> AttributePointer;

typedef QPair<QByteArray, QByteArray> ScopeNamePair;
typedef QPair<QByteArray, int> NameIntPair;
typedef QSharedPointer<ObjectStreamer> ObjectStreamerPointer;
typedef QWeakPointer<ObjectStreamer> WeakObjectStreamerPointer;
typedef QSharedPointer<TypeStreamer> TypeStreamerPointer;
typedef QWeakPointer<TypeStreamer> WeakTypeStreamerPointer;

typedef QPair<QVariant, QVariant> QVariantPair;
typedef QList<QVariantPair> QVariantPairList;

Q_DECLARE_METATYPE(QVariantPairList)

/// Streams integer identifiers that conform to the following pattern: each ID encountered in the stream is either one that
/// has been sent (received) before, or is one more than the highest previously encountered ID (starting at zero).  This allows
/// us to use the minimum number of bits to encode the IDs.
class IDStreamer {
public:
    
    IDStreamer(Bitstream& stream);
    
    void setBitsFromValue(int value);
    
    IDStreamer& operator<<(int value);
    IDStreamer& operator>>(int& value);
    
private:
    
    Bitstream& _stream;
    int _bits;
};

/// Provides a means to stream repeated values efficiently.  The value is first streamed along with a unique ID.  When
/// subsequently streamed, only the ID is sent.
template<class K, class P = K, class V = K> class RepeatedValueStreamer {
public:
    
    RepeatedValueStreamer(Bitstream& stream) : _stream(stream), _idStreamer(stream),
        _lastPersistentID(0), _lastTransientOffset(0) { }
    
    QHash<K, int> getAndResetTransientOffsets();
    
    void persistTransientOffsets(const QHash<K, int>& transientOffsets);
    
    QHash<int, V> getAndResetTransientValues();
    
    void persistTransientValues(const QHash<int, V>& transientValues);
    
    void removePersistentID(P value) { _persistentIDs.remove(value); }
    
    int takePersistentID(P value) { return _persistentIDs.take(value); }
    
    void removePersistentValue(V value) { int id = _valueIDs.take(value); _persistentValues.remove(id); }
    
    V takePersistentValue(int id) { V value = _persistentValues.take(id); _valueIDs.remove(value); return value; }
    
    void copyPersistentMappings(const RepeatedValueStreamer& other);
    void clearPersistentMappings();
    
    RepeatedValueStreamer& operator<<(K value);
    RepeatedValueStreamer& operator>>(V& value);
    
private:
    
    Bitstream& _stream;
    IDStreamer _idStreamer;
    int _lastPersistentID;
    int _lastTransientOffset;
    QHash<P, int> _persistentIDs;
    QHash<K, int> _transientOffsets;
    QHash<int, V> _persistentValues;
    QHash<int, V> _transientValues;
    QHash<V, int> _valueIDs;
};

template<class K, class P, class V> inline QHash<K, int> RepeatedValueStreamer<K, P, V>::getAndResetTransientOffsets() {
    QHash<K, int> transientOffsets;
    _transientOffsets.swap(transientOffsets);
    _lastTransientOffset = 0;
    _idStreamer.setBitsFromValue(_lastPersistentID);
    return transientOffsets;
}

template<class K, class P, class V> inline void RepeatedValueStreamer<K, P, V>::persistTransientOffsets(
        const QHash<K, int>& transientOffsets) {
    int oldLastPersistentID = _lastPersistentID;
    for (typename QHash<K, int>::const_iterator it = transientOffsets.constBegin(); it != transientOffsets.constEnd(); it++) {
        int& id = _persistentIDs[it.key()];
        if (id == 0) {
            id = oldLastPersistentID + it.value();
            _lastPersistentID = qMax(_lastPersistentID, id);
        }
    }
    _idStreamer.setBitsFromValue(_lastPersistentID);
}

template<class K, class P, class V> inline QHash<int, V> RepeatedValueStreamer<K, P, V>::getAndResetTransientValues() {
    QHash<int, V> transientValues;
    _transientValues.swap(transientValues);
    _idStreamer.setBitsFromValue(_lastPersistentID);
    return transientValues;
}

template<class K, class P, class V> inline void RepeatedValueStreamer<K, P, V>::persistTransientValues(
        const QHash<int, V>& transientValues) {
    int oldLastPersistentID = _lastPersistentID;
    for (typename QHash<int, V>::const_iterator it = transientValues.constBegin(); it != transientValues.constEnd(); it++) {
        int& id = _valueIDs[it.value()];
        if (id == 0) {
            id = oldLastPersistentID + it.key();
            _lastPersistentID = qMax(_lastPersistentID, id);
            _persistentValues.insert(id, it.value());
        }
    }
    _idStreamer.setBitsFromValue(_lastPersistentID);
}

template<class K, class P, class V> inline RepeatedValueStreamer<K, P, V>&
        RepeatedValueStreamer<K, P, V>::operator<<(K value) {
    int id = _persistentIDs.value(value);
    if (id == 0) {
        int& offset = _transientOffsets[value];
        if (offset == 0) {
            _idStreamer << (_lastPersistentID + (offset = ++_lastTransientOffset));
            _stream < value;
            
        } else {
            _idStreamer << (_lastPersistentID + offset);
        }
    } else {
        _idStreamer << id;
    }
    return *this;
}

template<class K, class P, class V> inline RepeatedValueStreamer<K, P, V>&
        RepeatedValueStreamer<K, P, V>::operator>>(V& value) {
    int id;
    _idStreamer >> id;
    if (id <= _lastPersistentID) {
        value = _persistentValues.value(id);
        
    } else {
        int offset = id - _lastPersistentID;
        typename QHash<int, V>::iterator it = _transientValues.find(offset);
        if (it == _transientValues.end()) {
            _stream > value;
            _transientValues.insert(offset, value);
        
        } else {
            value = *it;
        }
    }
    return *this;
}

template<class K, class P, class V> inline void RepeatedValueStreamer<K, P, V>::copyPersistentMappings(
        const RepeatedValueStreamer<K, P, V>& other) {
    _lastPersistentID = other._lastPersistentID;
    _idStreamer.setBitsFromValue(_lastPersistentID);
    _persistentIDs = other._persistentIDs;
    _transientOffsets.clear();
    _lastTransientOffset = 0;
    _persistentValues = other._persistentValues;
    _transientValues.clear();
    _valueIDs = other._valueIDs;
}

template<class K, class P, class V> inline void RepeatedValueStreamer<K, P, V>::clearPersistentMappings() {
    _lastPersistentID = 0;
    _idStreamer.setBitsFromValue(_lastPersistentID);
    _persistentIDs.clear();
    _transientOffsets.clear();
    _lastTransientOffset = 0;
    _persistentValues.clear();
    _transientValues.clear();
    _valueIDs.clear();
}

/// A stream for bit-aligned data.  Through a combination of code generation, reflection, macros, and templates, provides a
/// serialization mechanism that may be used for both networking and persistent storage.  For unreliable networking, the
/// class provides a mapping system that resends mappings for ids until they are acknowledged (and thus persisted).  For
/// version-resilient persistence, the class provides a metadata system that maps stored types to local types (or to
/// generic containers), allowing one to add and remove fields to classes without breaking compatibility with previously
/// stored data.
///
/// The basic usage requires one to create a Bitstream that wraps an underlying QDataStream, specifying the metadata type
/// desired and (for readers) the generics mode.  Then, one uses the << or >> operators to write or read values to/from
/// the stream (a stream instance may be used for reading or writing, but not both).  For write streams, the flush
/// function should be called on completion to write any partial data.
///
/// Polymorphic types are supported via the QVariant and QObject*/SharedObjectPointer types.  When you write a QVariant or
/// QObject, the type or class name (at minimum) is written to the stream.  When you read a QVariant or QObject, the default
/// behavior is to look for a corresponding registered type with the written name.  With hash metadata, Bitstream can verify
/// that the local type matches the stream type, throwing the streamed version away if not.  With full metadata, Bitstream can
/// create a mapping from the streamed type to the local type that applies the intersection of the two types' fields.
///
/// To register types for streaming, select from the provided templates and macros.  To register a QObject (or SharedObject)
/// subclass, use REGISTER_META_OBJECT in the class's source file.  To register a streamable class for use in QVariant, use
/// the STREAMABLE/STREAM/DECLARE_STREAMABLE_METATYPE macros and use the mtc tool to generate the associated implementation
/// code.  To register a QObject enum for use outside the QObject's direct properties, use the
/// DECLARE_ENUM_METATYPE/IMPLEMENT_ENUM_METATYPE macro pair.  To register a collection type (QList, QVector, QSet, QMap) of
/// streamable types, use the registerCollectionMetaType template function.
///
/// Delta-streaming is supported through the writeDelta/readDelta functions (analogous to <</>>), which accept a reference
/// value and stream only the information necessary to turn the reference into the target value.  This assumes that the
/// reference value provided when reading is identical to the one used when writing.
///
/// Special delta handling is provided for objects tracked by SharedObjectPointers.  SharedObjects have IDs (representing their
/// unique identity on one system) as well as origin IDs (representing their identity as preserved over the course of
/// mutation).  Once a shared object with a given ID is written (and its mapping persisted), that object's state will not be
/// written again; only its ID will be sent.  However, if an object with a new ID but the same origin ID is written, that
/// object's state will be encoded as a delta between the previous object and the new one.  So, to transmit delta-encoded
/// objects, one should treat each local SharedObject instance as immutable, replacing it when mutated with a cloned instance
/// generated by calling SharedObject::clone(true) and applying the desired changes.
class Bitstream : public QObject {
    Q_OBJECT

public:

    /// Stores a set of mappings from values to ids written.  Typically, one would store these mappings along with the send
    /// record of the packet that contained them, persisting the mappings if/when the packet is acknowledged by the remote
    /// party.
    class WriteMappings {
    public:
        QHash<const ObjectStreamer*, int> objectStreamerOffsets;
        QHash<const TypeStreamer*, int> typeStreamerOffsets;
        QHash<AttributePointer, int> attributeOffsets;
        QHash<QScriptString, int> scriptStringOffsets;
        QHash<SharedObjectPointer, int> sharedObjectOffsets;
    };

    /// Stores a set of mappings from ids to values read.  Typically, one would store these mappings along with the receive
    /// record of the packet that contained them, persisting the mappings if/when the remote party indicates that it
    /// has received the local party's acknowledgement of the packet.
    class ReadMappings {
    public:
        QHash<int, ObjectStreamerPointer> objectStreamerValues;
        QHash<int, TypeStreamerPointer> typeStreamerValues;
        QHash<int, AttributePointer> attributeValues;
        QHash<int, QScriptString> scriptStringValues;
        QHash<int, SharedObjectPointer> sharedObjectValues;
    };

    /// Performs all of the various lazily initializations (of object streamers, etc.)  If multiple threads need to use
    /// Bitstream instances, call this beforehand to prevent errors from occurring when multiple threads attempt lazy
    /// initialization simultaneously.
    static void preThreadingInit();

    /// Registers a metaobject under its name so that instances of it can be streamed.  Consider using the REGISTER_META_OBJECT
    /// at the top level of the source file associated with the class rather than calling this function directly.
    /// \return zero; the function only returns a value so that it can be used in static initialization
    static int registerMetaObject(const char* className, const QMetaObject* metaObject);

    /// Registers a streamer for the specified Qt-registered type.  Consider using one of the registration macros (such as
    /// REGISTER_SIMPLE_TYPE_STREAMER) at the top level of the associated source file rather than calling this function
    /// directly. 
    /// \return zero; the function only returns a value so that it can be used in static initialization
    static int registerTypeStreamer(int type, TypeStreamer* streamer);

    /// Returns the streamer registered for the supplied type, if any.
    static const TypeStreamer* getTypeStreamer(int type);

    /// Returns the streamer registered for the supplied object, if any.
    static const ObjectStreamer* getObjectStreamer(const QMetaObject* metaObject);

    /// Returns the meta-object registered under the supplied class name, if any.
    static const QMetaObject* getMetaObject(const QByteArray& className);

    /// Returns the list of registered subclasses for the supplied meta-object.  When registered, metaobjects register
    /// themselves as subclasses of all of their parents, mostly in order to allow editors to provide lists of available
    /// subclasses.
    static QList<const QMetaObject*> getMetaObjectSubClasses(const QMetaObject* metaObject);

    /// Configures the supplied script engine with our registered meta-objects, allowing all of them to be instantiated from
    /// scripts.
    static void registerTypes(QScriptEngine* engine);

    enum MetadataType { NO_METADATA, HASH_METADATA, FULL_METADATA };

    enum GenericsMode { NO_GENERICS, FALLBACK_GENERICS, ALL_GENERICS }; 

    /// Creates a new bitstream.  Note: the stream may be used for reading or writing, but not both.
    /// \param metadataType the metadata type, which determines the amount of version-resiliency: with no metadata, all types
    /// must match exactly; with hash metadata, any types which do not match exactly will be ignored; with full metadata,
    /// fields (and enum values, etc.) will be remapped by name
    /// \param genericsMode the generics mode, which determines which types will be replaced by generic equivalents: with
    /// no generics, no generics will be created; with fallback generics, generics will be created for any unknown types; with
    /// all generics, generics will be created for all non-simple types
    Bitstream(QDataStream& underlying, MetadataType metadataType = NO_METADATA,
        GenericsMode = NO_GENERICS, QObject* parent = NULL);

    /// Returns a reference to the underlying data stream.
    QDataStream& getUnderlying() { return _underlying; }

    /// Substitutes the supplied metaobject for the given class name's default mapping.  This is mostly useful for testing the
    /// process of mapping between different types, but may in the future be used for permanently renaming classes.
    void addMetaObjectSubstitution(const QByteArray& className, const QMetaObject* metaObject);
    
    /// Substitutes the supplied type for the given type name's default mapping.
    void addTypeSubstitution(const QByteArray& typeName, int type);

    /// Substitutes the named type for the given type name's default mapping.
    void addTypeSubstitution(const QByteArray& typeName, const char* replacementTypeName);
    
    /// Writes a set of bits to the underlying stream.
    /// \param bits the number of bits to write
    /// \param offset the offset of the first bit
    Bitstream& write(const void* data, int bits, int offset = 0);
    
    /// Reads a set of bits from the underlying stream.
    /// \param bits the number of bits to read
    /// \param offset the offset of the first bit
    Bitstream& read(void* data, int bits, int offset = 0);    

    /// Flushes any unwritten bits to the underlying stream.
    void flush();

    /// Resets to the initial state.
    void reset();

    /// Returns the set of transient mappings gathered during writing and resets them.
    WriteMappings getAndResetWriteMappings();

    /// Persists a set of write mappings recorded earlier.
    void persistWriteMappings(const WriteMappings& mappings);

    /// Immediately persists and resets the write mappings.
    void persistAndResetWriteMappings();

    /// Returns the set of transient mappings gathered during reading and resets them.
    ReadMappings getAndResetReadMappings();
    
    /// Persists a set of read mappings recorded earlier.
    void persistReadMappings(const ReadMappings& mappings);

    /// Immediately persists and resets the read mappings.
    void persistAndResetReadMappings();

    /// Copies the persistent mappings from the specified other stream.
    void copyPersistentMappings(const Bitstream& other);

    /// Clears the persistent mappings for this stream.
    void clearPersistentMappings();

    /// Returns a reference to the weak hash storing shared objects for this stream.
    const WeakSharedObjectHash& getWeakSharedObjectHash() const { return _weakSharedObjectHash; }

    /// Removes a shared object from the read mappings.
    void clearSharedObject(int id);

    void writeDelta(bool value, bool reference);
    void readDelta(bool& value, bool reference);

    void writeDelta(const QVariant& value, const QVariant& reference);
    
    template<class T> void writeDelta(const T& value, const T& reference);
    template<class T> void readDelta(T& value, const T& reference); 

    void writeRawDelta(const QVariant& value, const QVariant& reference);
    void readRawDelta(QVariant& value, const QVariant& reference);

    void writeRawDelta(const QObject* value, const QObject* reference);
    void readRawDelta(QObject*& value, const QObject* reference);

    void writeRawDelta(const QScriptValue& value, const QScriptValue& reference);
    void readRawDelta(QScriptValue& value, const QScriptValue& reference);

    template<class T> void writeRawDelta(const T& value, const T& reference);
    template<class T> void readRawDelta(T& value, const T& reference); 
    
    template<class T> void writeRawDelta(const QList<T>& value, const QList<T>& reference);
    template<class T> void readRawDelta(QList<T>& value, const QList<T>& reference); 
    
    template<class T> void writeRawDelta(const QVector<T>& value, const QVector<T>& reference);
    template<class T> void readRawDelta(QVector<T>& value, const QVector<T>& reference); 

    template<class T> void writeRawDelta(const QSet<T>& value, const QSet<T>& reference);
    template<class T> void readRawDelta(QSet<T>& value, const QSet<T>& reference); 
    
    template<class K, class V> void writeRawDelta(const QHash<K, V>& value, const QHash<K, V>& reference);
    template<class K, class V> void readRawDelta(QHash<K, V>& value, const QHash<K, V>& reference);
    
    /// Writes the specified array aligned on byte boundaries to avoid the inefficiency
    /// of bit-twiddling (at the cost of up to seven bits of wasted space).
    void writeAligned(const QByteArray& data);
    QByteArray readAligned(int bytes);
    
    Bitstream& operator<<(bool value);
    Bitstream& operator>>(bool& value);
    
    Bitstream& operator<<(int value);
    Bitstream& operator>>(int& value);
    
    Bitstream& operator<<(uint value);
    Bitstream& operator>>(uint& value);
    
    Bitstream& operator<<(qint64 value);
    Bitstream& operator>>(qint64& value);
    
    Bitstream& operator<<(float value);
    Bitstream& operator>>(float& value);
    
    Bitstream& operator<<(double value);
    Bitstream& operator>>(double& value);
    
    Bitstream& operator<<(const glm::vec3& value);
    Bitstream& operator>>(glm::vec3& value);
    
    Bitstream& operator<<(const glm::quat& value);
    Bitstream& operator>>(glm::quat& value);
    
    Bitstream& operator<<(const QByteArray& string);
    Bitstream& operator>>(QByteArray& string);
    
    Bitstream& operator<<(const QColor& color);
    Bitstream& operator>>(QColor& color);
    
    Bitstream& operator<<(const QString& string);
    Bitstream& operator>>(QString& string);
    
    Bitstream& operator<<(const QUrl& url);
    Bitstream& operator>>(QUrl& url);
    
    Bitstream& operator<<(const QDateTime& dateTime);
    Bitstream& operator>>(QDateTime& dateTime);
    
    Bitstream& operator<<(const QRegExp& regExp);
    Bitstream& operator>>(QRegExp& regExp);
    
    Bitstream& operator<<(const QVariant& value);
    Bitstream& operator>>(QVariant& value);
    
    Bitstream& operator<<(const AttributeValue& attributeValue);
    Bitstream& operator>>(OwnedAttributeValue& attributeValue);
    
    Bitstream& operator<<(const GenericValue& value);
    Bitstream& operator>>(GenericValue& value);
    
    template<class T> Bitstream& operator<<(const QList<T>& list);
    template<class T> Bitstream& operator>>(QList<T>& list);
    
    template<class T> Bitstream& operator<<(const QVector<T>& list);
    template<class T> Bitstream& operator>>(QVector<T>& list);

    template<class T> Bitstream& operator<<(const QSet<T>& set);
    template<class T> Bitstream& operator>>(QSet<T>& set);
    
    template<class K, class V> Bitstream& operator<<(const QHash<K, V>& hash);
    template<class K, class V> Bitstream& operator>>(QHash<K, V>& hash);
    
    Bitstream& operator<<(const QObject* object);
    Bitstream& operator>>(QObject*& object);
    
    Bitstream& operator<<(const QMetaObject* metaObject);
    Bitstream& operator>>(const QMetaObject*& metaObject);
    
    Bitstream& operator<<(const ObjectStreamer* streamer);
    Bitstream& operator>>(const ObjectStreamer*& streamer);
    Bitstream& operator>>(ObjectStreamerPointer& streamer);
    
    Bitstream& operator<<(const TypeStreamer* streamer);
    Bitstream& operator>>(const TypeStreamer*& streamer);
    Bitstream& operator>>(TypeStreamerPointer& streamer);
    
    Bitstream& operator<<(const AttributePointer& attribute);
    Bitstream& operator>>(AttributePointer& attribute);
    
    Bitstream& operator<<(const QScriptString& string);
    Bitstream& operator>>(QScriptString& string);
    
    Bitstream& operator<<(const QScriptValue& value);
    Bitstream& operator>>(QScriptValue& value);
    
    Bitstream& operator<<(const SharedObjectPointer& object);
    Bitstream& operator>>(SharedObjectPointer& object);
    
    Bitstream& operator<(const ObjectStreamer* streamer);
    Bitstream& operator>(ObjectStreamerPointer& streamer);
    
    Bitstream& operator<(const TypeStreamer* streamer);
    Bitstream& operator>(TypeStreamerPointer& streamer);
    
    Bitstream& operator<(const AttributePointer& attribute);
    Bitstream& operator>(AttributePointer& attribute);
    
    Bitstream& operator<(const QScriptString& string);
    Bitstream& operator>(QScriptString& string);
    
    Bitstream& operator<(const SharedObjectPointer& object);
    Bitstream& operator>(SharedObjectPointer& object);

signals:

    void sharedObjectCleared(int id);

private slots:
    
    void clearSharedObject(QObject* object);

private:
    
    friend class JSONReader;
    friend class JSONWriter;
    
    ObjectStreamerPointer readGenericObjectStreamer(const QByteArray& name);
    TypeStreamerPointer readGenericTypeStreamer(const QByteArray& name, int category);
    
    QDataStream& _underlying;
    quint8 _byte;
    int _position;

    MetadataType _metadataType;
    GenericsMode _genericsMode;

    RepeatedValueStreamer<const ObjectStreamer*, const ObjectStreamer*, ObjectStreamerPointer> _objectStreamerStreamer;
    RepeatedValueStreamer<const TypeStreamer*, const TypeStreamer*, TypeStreamerPointer> _typeStreamerStreamer;
    RepeatedValueStreamer<AttributePointer> _attributeStreamer;
    RepeatedValueStreamer<QScriptString> _scriptStringStreamer;
    RepeatedValueStreamer<SharedObjectPointer, SharedObject*> _sharedObjectStreamer;
    
    WeakSharedObjectHash _sharedObjectReferences;

    WeakSharedObjectHash _weakSharedObjectHash;

    QHash<QByteArray, const QMetaObject*> _metaObjectSubstitutions;
    QHash<QByteArray, const TypeStreamer*> _typeStreamerSubstitutions;

    static QHash<QByteArray, const QMetaObject*>& getMetaObjects();
    static QMultiHash<const QMetaObject*, const QMetaObject*>& getMetaObjectSubClasses();
    static QHash<int, const TypeStreamer*>& getTypeStreamers();
         
    static const QHash<const QMetaObject*, const ObjectStreamer*>& getObjectStreamers();
    static QHash<const QMetaObject*, const ObjectStreamer*> createObjectStreamers();
    
    static const QHash<ScopeNamePair, const TypeStreamer*>& getEnumStreamers();
    static QHash<ScopeNamePair, const TypeStreamer*> createEnumStreamers();
    
    static const QHash<QByteArray, const TypeStreamer*>& getEnumStreamersByName();
    static QHash<QByteArray, const TypeStreamer*> createEnumStreamersByName();
    
    static const TypeStreamer* getInvalidTypeStreamer();
    static const TypeStreamer* createInvalidTypeStreamer();
};

template<class T> inline void Bitstream::writeDelta(const T& value, const T& reference) {
    if (value == reference) {
        *this << false;
    } else {
        *this << true;
        writeRawDelta(value, reference);
    }
}

template<class T> inline void Bitstream::readDelta(T& value, const T& reference) {
    bool changed;
    *this >> changed;
    if (changed) {
        readRawDelta(value, reference);
    } else {
        value = reference;
    }
}

template<class T> inline void Bitstream::writeRawDelta(const T& value, const T& reference) {
    *this << value;
}

template<class T> inline void Bitstream::readRawDelta(T& value, const T& reference) {
    *this >> value;
}

template<class T> inline void Bitstream::writeRawDelta(const QList<T>& value, const QList<T>& reference) {
    *this << value.size();
    *this << reference.size();
    for (int i = 0; i < value.size(); i++) {
        if (i < reference.size()) {
            writeDelta(value.at(i), reference.at(i));    
        } else {
            *this << value.at(i);
        }
    }
}

template<class T> inline void Bitstream::readRawDelta(QList<T>& value, const QList<T>& reference) {
    value = reference;
    int size, referenceSize;
    *this >> size >> referenceSize;
    if (size < value.size()) {
        value.erase(value.begin() + size, value.end());
    }
    for (int i = 0; i < size; i++) {
        if (i < referenceSize) {
            readDelta(value[i], reference.at(i));
        } else {
            T element;
            *this >> element;
            value.append(element);
        }
    }
}

template<class T> inline void Bitstream::writeRawDelta(const QVector<T>& value, const QVector<T>& reference) {
    *this << value.size();
    *this << reference.size();
    for (int i = 0; i < value.size(); i++) {
        if (i < reference.size()) {
            writeDelta(value.at(i), reference.at(i));    
        } else {
            *this << value.at(i);
        }
    }
}

template<class T> inline void Bitstream::readRawDelta(QVector<T>& value, const QVector<T>& reference) {
    value = reference;
    int size, referenceSize;
    *this >> size >> referenceSize;
    if (size < value.size()) {
        value.erase(value.begin() + size, value.end());
    }
    for (int i = 0; i < size; i++) {
        if (i < referenceSize) {
            readDelta(value[i], reference.at(i));
        } else {
            T element;
            *this >> element;
            value.append(element);
        }
    }
}

template<class T> inline void Bitstream::writeRawDelta(const QSet<T>& value, const QSet<T>& reference) {
    int addedOrRemoved = 0;
    foreach (const T& element, value) {
        if (!reference.contains(element)) {
            addedOrRemoved++;
        }
    }
    foreach (const T& element, reference) {
        if (!value.contains(element)) {
            addedOrRemoved++;
        }
    }
    *this << addedOrRemoved;
    foreach (const T& element, value) {
        if (!reference.contains(element)) {
            *this << element;
        }
    }
    foreach (const T& element, reference) {
        if (!value.contains(element)) {
            *this << element;
        }
    }
}

template<class T> inline void Bitstream::readRawDelta(QSet<T>& value, const QSet<T>& reference) {
    value = reference;
    int addedOrRemoved;
    *this >> addedOrRemoved;
    for (int i = 0; i < addedOrRemoved; i++) {
        T element;
        *this >> element;
        if (!value.remove(element)) {
            value.insert(element);
        }
    }
}

template<class K, class V> inline void Bitstream::writeRawDelta(const QHash<K, V>& value, const QHash<K, V>& reference) {
    int added = 0;
    int modified = 0;
    for (typename QHash<K, V>::const_iterator it = value.constBegin(); it != value.constEnd(); it++) {
        typename QHash<K, V>::const_iterator previous = reference.find(it.key());
        if (previous == reference.constEnd()) {
            added++;
        } else if (previous.value() != it.value()) {
            modified++;
        }
    }
    *this << added;
    for (typename QHash<K, V>::const_iterator it = value.constBegin(); it != value.constEnd(); it++) {
        if (!reference.contains(it.key())) {
            *this << it.key();
            *this << it.value();
        }
    }
    *this << modified;
    for (typename QHash<K, V>::const_iterator it = value.constBegin(); it != value.constEnd(); it++) {
        typename QHash<K, V>::const_iterator previous = reference.find(it.key());
        if (previous != reference.constEnd() && previous.value() != it.value()) {
            *this << it.key();
            writeDelta(it.value(), previous.value());
        }
    }
    int removed = 0;
    for (typename QHash<K, V>::const_iterator it = reference.constBegin(); it != reference.constEnd(); it++) {
        if (!value.contains(it.key())) {
            removed++;
        }
    }
    *this << removed;
    for (typename QHash<K, V>::const_iterator it = reference.constBegin(); it != reference.constEnd(); it++) {
        if (!value.contains(it.key())) {
            *this << it.key();
        }
    }
}

template<class K, class V> inline void Bitstream::readRawDelta(QHash<K, V>& value, const QHash<K, V>& reference) {
    value = reference;
    int added;
    *this >> added;
    for (int i = 0; i < added; i++) {
        K key;
        V mapping;
        *this >> key >> mapping;
        value.insert(key, mapping);
    }
    int modified;
    *this >> modified;
    for (int i = 0; i < modified; i++) {
        K key;
        *this >> key;
        V& mapping = value[key];
        V newMapping;
        readDelta(newMapping, mapping);
        mapping = newMapping;
    }
    int removed;
    *this >> removed;
    for (int i = 0; i < removed; i++) {
        K key;
        *this >> key;
        value.remove(key);
    }
}

template<class T> inline Bitstream& Bitstream::operator<<(const QList<T>& list) {
    *this << list.size();
    foreach (const T& entry, list) {
        *this << entry;
    }
    return *this;
}

template<class T> inline Bitstream& Bitstream::operator>>(QList<T>& list) {
    int size;
    *this >> size;
    list.clear();
    list.reserve(size);
    for (int i = 0; i < size; i++) {
        T entry;
        *this >> entry;
        list.append(entry);
    }
    return *this;
}

template<class T> inline Bitstream& Bitstream::operator<<(const QVector<T>& vector) {
    *this << vector.size();
    foreach (const T& entry, vector) {
        *this << entry;
    }
    return *this;
}

template<class T> inline Bitstream& Bitstream::operator>>(QVector<T>& vector) {
    int size;
    *this >> size;
    vector.clear();
    vector.reserve(size);
    for (int i = 0; i < size; i++) {
        T entry;
        *this >> entry;
        vector.append(entry);
    }
    return *this;
}

template<class T> inline Bitstream& Bitstream::operator<<(const QSet<T>& set) {
    *this << set.size();
    foreach (const T& entry, set) {
        *this << entry;
    }
    return *this;
}

template<class T> inline Bitstream& Bitstream::operator>>(QSet<T>& set) {
    int size;
    *this >> size;
    set.clear();
    set.reserve(size);
    for (int i = 0; i < size; i++) {
        T entry;
        *this >> entry;
        set.insert(entry);
    }
    return *this;
}

template<class K, class V> inline Bitstream& Bitstream::operator<<(const QHash<K, V>& hash) {
    *this << hash.size();
    for (typename QHash<K, V>::const_iterator it = hash.constBegin(); it != hash.constEnd(); it++) {
        *this << it.key();
        *this << it.value();
    }
    return *this;
}

template<class K, class V> inline Bitstream& Bitstream::operator>>(QHash<K, V>& hash) {
    int size;
    *this >> size;
    hash.clear();
    hash.reserve(size);
    for (int i = 0; i < size; i++) {
        K key;
        V value;
        *this >> key;
        *this >> value;
        hash.insertMulti(key, value);
    }
    return *this;
}

/// Thrown for unrecoverable errors.
class BitstreamException {
public:
    
    BitstreamException(const QString& description);

    const QString& getDescription() const { return _description; }

private:
    
    QString _description;
};

/// Provides a means of writing Bitstream-able data to JSON rather than the usual binary format in a manner that allows it to
/// be manipulated and re-read, converted to binary, etc.  To use, create a JSONWriter, stream values in using the << operator,
/// and call getDocument to obtain the JSON data.
class JSONWriter {
public:
    
    QJsonValue getData(bool value);
    QJsonValue getData(int value);
    QJsonValue getData(uint value);
    QJsonValue getData(float value);
    QJsonValue getData(const QByteArray& value);
    QJsonValue getData(const QColor& value);
    QJsonValue getData(const QScriptValue& value);
    QJsonValue getData(const QString& value);
    QJsonValue getData(const QUrl& value);
    QJsonValue getData(const QDateTime& value);
    QJsonValue getData(const QRegExp& value);
    QJsonValue getData(const glm::vec3& value);
    QJsonValue getData(const glm::quat& value);
    QJsonValue getData(const QMetaObject* value);
    QJsonValue getData(const QVariant& value);
    QJsonValue getData(const SharedObjectPointer& value);
    QJsonValue getData(const QObject* value);
    QJsonValue getData(const GenericValue& value);
    
    template<class T> QJsonValue getData(const T& value) { return QJsonValue(); }
    
    template<class T> QJsonValue getData(const QList<T>& list);
    template<class T> QJsonValue getData(const QVector<T>& list);
    template<class T> QJsonValue getData(const QSet<T>& set);
    template<class K, class V> QJsonValue getData(const QHash<K, V>& hash);
    
    template<class T> JSONWriter& operator<<(const T& value) { _contents.append(getData(value)); return *this; }
    
    void appendToContents(const QJsonValue& value) { _contents.append(value); }
    
    void addSharedObject(const SharedObjectPointer& object);
    void addObjectStreamer(const ObjectStreamer* streamer);
    void addTypeStreamer(const TypeStreamer* streamer);
    
    QJsonDocument getDocument() const;
    
private:

    QJsonArray _contents;

    QSet<int> _sharedObjectIDs;
    QJsonArray _sharedObjects;
    
    QSet<QByteArray> _objectStreamerNames;
    QJsonArray _objectStreamers;
    
    QSet<QByteArray> _typeStreamerNames;
    QJsonArray _typeStreamers;
};

template<class T> inline QJsonValue JSONWriter::getData(const QList<T>& list) {
    QJsonArray array;
    foreach (const T& value, list) {
        array.append(getData(value));
    }
    return array;
}

template<class T> inline QJsonValue JSONWriter::getData(const QVector<T>& vector) {
    QJsonArray array;
    foreach (const T& value, vector) {
        array.append(getData(value));
    }
    return array;
}

template<class T> inline QJsonValue JSONWriter::getData(const QSet<T>& set) {
    QJsonArray array;
    foreach (const T& value, set) {
        array.append(getData(value));
    }
    return array;
}

template<class K, class V> inline QJsonValue JSONWriter::getData(const QHash<K, V>& hash) {
    QJsonArray array;
    for (typename QHash<K, V>::const_iterator it = hash.constBegin(); it != hash.constEnd(); it++) {
        QJsonArray pair;
        pair.append(getData(it.key()));
        pair.append(getData(it.value()));
        array.append(pair);
    }
    return array;
}

/// Reads a document written by JSONWriter.  To use, create a JSONReader and stream values out using the >> operator.
class JSONReader {
public:
    
    /// Creates a reader to read from the supplied document.
    /// \param genericsMode the generics mode to use: NO_GENERICS to map all types in the document to built-in types,
    /// FALLBACK_GENERICS to use generic containers where no matching built-in type is found, or ALL_GENERICS to
    /// read all types to generic containers
    JSONReader(const QJsonDocument& document, Bitstream::GenericsMode genericsMode = Bitstream::NO_GENERICS);
    
    void putData(const QJsonValue& data, bool& value);
    void putData(const QJsonValue& data, int& value);
    void putData(const QJsonValue& data, uint& value);
    void putData(const QJsonValue& data, float& value);
    void putData(const QJsonValue& data, QByteArray& value);
    void putData(const QJsonValue& data, QColor& value);
    void putData(const QJsonValue& data, QScriptValue& value);
    void putData(const QJsonValue& data, QString& value);
    void putData(const QJsonValue& data, QUrl& value);
    void putData(const QJsonValue& data, QDateTime& value);
    void putData(const QJsonValue& data, QRegExp& value);
    void putData(const QJsonValue& data, glm::vec3& value);
    void putData(const QJsonValue& data, glm::quat& value);
    void putData(const QJsonValue& data, const QMetaObject*& value);
    void putData(const QJsonValue& data, QVariant& value);
    void putData(const QJsonValue& data, SharedObjectPointer& value);
    void putData(const QJsonValue& data, QObject*& value);
    
    template<class T> void putData(const QJsonValue& data, T& value) { value = T(); }
    
    template<class T> void putData(const QJsonValue& data, QList<T>& list);
    template<class T> void putData(const QJsonValue& data, QVector<T>& list);
    template<class T> void putData(const QJsonValue& data, QSet<T>& set);
    template<class K, class V> void putData(const QJsonValue& data, QHash<K, V>& hash);
    
    template<class T> JSONReader& operator>>(T& value) { putData(*_contentsIterator++, value); return *this; }

    QJsonValue retrieveNextFromContents() { return *_contentsIterator++; }
    
    TypeStreamerPointer getTypeStreamer(const QString& name) const;
    ObjectStreamerPointer getObjectStreamer(const QString& name) const { return _objectStreamers.value(name); }
    SharedObjectPointer getSharedObject(int id) const { return _sharedObjects.value(id); }

private:
    
    QJsonArray _contents;
    QJsonArray::const_iterator _contentsIterator;
    
    QHash<QString, TypeStreamerPointer> _typeStreamers;
    QHash<QString, ObjectStreamerPointer> _objectStreamers;
    QHash<int, SharedObjectPointer> _sharedObjects;
};

template<class T> inline void JSONReader::putData(const QJsonValue& data, QList<T>& list) {
    list.clear();
    foreach (const QJsonValue& element, data.toArray()) {
        T value;
        putData(element, value);
        list.append(value);
    }
}

template<class T> inline void JSONReader::putData(const QJsonValue& data, QVector<T>& list) {
    list.clear();
    foreach (const QJsonValue& element, data.toArray()) {
        T value;
        putData(element, value);
        list.append(value);
    }
}

template<class T> inline void JSONReader::putData(const QJsonValue& data, QSet<T>& set) {
    set.clear();
    foreach (const QJsonValue& element, data.toArray()) {
        T value;
        putData(element, value);
        set.insert(value);
    }
}

template<class K, class V> inline void JSONReader::putData(const QJsonValue& data, QHash<K, V>& hash) {
    hash.clear();
    foreach (const QJsonValue& element, data.toArray()) {
        QJsonArray pair = element.toArray();
        K key;
        putData(pair.at(0), key);
        V value;
        putData(pair.at(1), value);
        hash.insert(key, value);
    }
}
    
typedef QPair<TypeStreamerPointer, QMetaProperty> StreamerPropertyPair;

/// Contains the information required to stream an object.
class ObjectStreamer {
public:

    ObjectStreamer(const QMetaObject* metaObject);
    virtual ~ObjectStreamer();

    const QMetaObject* getMetaObject() const { return _metaObject; }
    const ObjectStreamerPointer& getSelf() const { return _self; }
    
    virtual const char* getName() const = 0;
    virtual const QVector<StreamerPropertyPair>& getProperties() const;
    virtual void writeMetadata(Bitstream& out, bool full) const = 0;
    virtual QJsonObject getJSONMetadata(JSONWriter& writer) const = 0;
    virtual QJsonObject getJSONData(JSONWriter& writer, const QObject* object) const = 0;
    virtual QObject* putJSONData(JSONReader& reader, const QJsonObject& jsonObject) const = 0;
    
    virtual bool equal(const QObject* first, const QObject* second) const = 0;
    virtual void write(Bitstream& out, const QObject* object) const = 0;
    virtual void writeRawDelta(Bitstream& out, const QObject* object, const QObject* reference) const = 0;
    virtual QObject* read(Bitstream& in, QObject* object = NULL) const = 0;
    virtual QObject* readRawDelta(Bitstream& in, const QObject* reference, QObject* object = NULL) const = 0;
    
protected:
    
    friend class Bitstream;

    const QMetaObject* _metaObject;
    ObjectStreamerPointer _self; ///< set/used for built-in classes (never deleted), to obtain shared pointers
};

/// A streamer that maps to a local class.
class MappedObjectStreamer : public ObjectStreamer {
public:

    MappedObjectStreamer(const QMetaObject* metaObject, const QVector<StreamerPropertyPair>& properties);

    virtual const char* getName() const;
    virtual const QVector<StreamerPropertyPair>& getProperties() const;
    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonObject getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonObject getJSONData(JSONWriter& writer, const QObject* object) const;
    virtual QObject* putJSONData(JSONReader& reader, const QJsonObject& jsonObject) const;
    virtual bool equal(const QObject* first, const QObject* second) const;
    virtual void write(Bitstream& out, const QObject* object) const;
    virtual void writeRawDelta(Bitstream& out, const QObject* object, const QObject* reference) const;
    virtual QObject* read(Bitstream& in, QObject* object = NULL) const;
    virtual QObject* readRawDelta(Bitstream& in, const QObject* reference, QObject* object = NULL) const;
    
private:
    
    QVector<StreamerPropertyPair> _properties;
};

typedef QPair<TypeStreamerPointer, QByteArray> StreamerNamePair;

/// A streamer for generic objects.
class GenericObjectStreamer : public ObjectStreamer {
public:

    GenericObjectStreamer(const QByteArray& name, const QVector<StreamerNamePair>& properties, const QByteArray& hash);

    virtual const char* getName() const;
    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonObject getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonObject getJSONData(JSONWriter& writer, const QObject* object) const;
    virtual QObject* putJSONData(JSONReader& reader, const QJsonObject& jsonObject) const;
    virtual bool equal(const QObject* first, const QObject* second) const;
    virtual void write(Bitstream& out, const QObject* object) const;
    virtual void writeRawDelta(Bitstream& out, const QObject* object, const QObject* reference) const;
    virtual QObject* read(Bitstream& in, QObject* object = NULL) const;
    virtual QObject* readRawDelta(Bitstream& in, const QObject* reference, QObject* object = NULL) const;
    
private:
    
    friend class Bitstream;
    friend class JSONReader;
    
    QByteArray _name;
    WeakObjectStreamerPointer _weakSelf; ///< promoted to strong references in GenericSharedObject when reading
    QVector<StreamerNamePair> _properties;
    QByteArray _hash;
};

/// Describes a metatype field.
class MetaField {
public:
    
    MetaField(const QByteArray& name = QByteArray(), const TypeStreamer* streamer = NULL);
    
    const QByteArray& getName() const { return _name; }
    const TypeStreamer* getStreamer() const { return _streamer; }
    
private:

    QByteArray _name;
    const TypeStreamer* _streamer;
};

Q_DECLARE_METATYPE(const QMetaObject*)

/// Macro for registering streamable meta-objects.  Typically, one would use this macro at the top level of the source file
/// associated with the class.  The class should have a no-argument constructor flagged with Q_INVOKABLE.
#define REGISTER_META_OBJECT(x) static int x##Registration = Bitstream::registerMetaObject(#x, &x::staticMetaObject);

/// Contains a value along with a pointer to its streamer.  This is stored in QVariants when using fallback generics and
/// no mapping to a built-in type can be found, or when using all generics and the value is any non-simple type.
class GenericValue {
public:
    
    GenericValue(const TypeStreamerPointer& streamer = TypeStreamerPointer(), const QVariant& value = QVariant());
    
    const TypeStreamerPointer& getStreamer() const { return _streamer; }
    const QVariant& getValue() const { return _value; }
    
    bool operator==(const GenericValue& other) const;
    
private:
    
    TypeStreamerPointer _streamer;
    QVariant _value;
};

Q_DECLARE_METATYPE(GenericValue)

/// Contains a list of property values along with a pointer to their metadata.  This is stored when using fallback generics
/// and no mapping to a built-in class can be found, or for all QObjects when using all generics.
class GenericSharedObject : public SharedObject {
    Q_OBJECT
    
public:

    GenericSharedObject(const ObjectStreamerPointer& streamer);

    const ObjectStreamerPointer& getStreamer() const { return _streamer; }

    void setValues(const QVariantList& values) { _values = values; }
    const QVariantList& getValues() const { return _values; }

private:
    
    ObjectStreamerPointer _streamer;
    QVariantList _values;
};

/// Interface for objects that can write values to and read values from bitstreams.
class TypeStreamer {
public:
    
    enum Category { SIMPLE_CATEGORY, ENUM_CATEGORY, STREAMABLE_CATEGORY, LIST_CATEGORY, SET_CATEGORY, MAP_CATEGORY };
    
    virtual ~TypeStreamer();
    
    int getType() const { return _type; }
    
    const TypeStreamerPointer& getSelf() const { return _self; }
    
    virtual const char* getName() const;
    
    virtual const TypeStreamer* getStreamerToWrite(const QVariant& value) const;
    
    virtual void writeMetadata(Bitstream& out, bool full) const;
    
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const;
    virtual QJsonValue getJSONVariantData(JSONWriter& writer, const QVariant& value) const;
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual void putJSONVariantData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    
    virtual bool equal(const QVariant& first, const QVariant& second) const;
    
    virtual void write(Bitstream& out, const QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;

    virtual void writeVariant(Bitstream& out, const QVariant& value) const;
    virtual QVariant readVariant(Bitstream& in) const;

    virtual void writeDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const;
    virtual void readDelta(Bitstream& in, QVariant& value, const QVariant& reference) const;

    virtual void writeRawDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const;
    virtual void readRawDelta(Bitstream& in, QVariant& value, const QVariant& reference) const;
    
    virtual void setEnumValue(QVariant& object, int value, const QHash<int, int>& mappings) const;
    
    virtual const QVector<MetaField>& getMetaFields() const;
    virtual int getFieldIndex(const QByteArray& name) const;
    virtual void setField(QVariant& object, int index, const QVariant& value) const;
    virtual QVariant getField(const QVariant& object, int index) const;
    
    virtual Category getCategory() const;

    virtual int getBits() const;
    virtual QMetaEnum getMetaEnum() const;

    virtual const TypeStreamer* getKeyStreamer() const;
    virtual const TypeStreamer* getValueStreamer() const;

    virtual void insert(QVariant& object, const QVariant& value) const;
    virtual void insert(QVariant& object, const QVariant& key, const QVariant& value) const;
    virtual bool remove(QVariant& object, const QVariant& key) const;
    
    virtual QVariant getValue(const QVariant& object, const QVariant& key) const;

    virtual void prune(QVariant& object, int size) const;
    virtual QVariant getValue(const QVariant& object, int index) const;
    virtual void setValue(QVariant& object, int index, const QVariant& value) const;

protected:
    
    friend class Bitstream;
    
    int _type;
    TypeStreamerPointer _self; ///< set/used for built-in types (never deleted), to obtain shared pointers
};

QDebug& operator<<(QDebug& debug, const TypeStreamer* typeStreamer);

QDebug& operator<<(QDebug& debug, const QMetaObject* metaObject);

/// A streamer that works with Bitstream's operators.
template<class T> class SimpleTypeStreamer : public TypeStreamer {
public:
    
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const {
        return writer.getData(value.value<T>()); }
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const {
        T rawValue; reader.putData(data, rawValue); value = QVariant::fromValue(rawValue); }
    virtual bool equal(const QVariant& first, const QVariant& second) const { return first.value<T>() == second.value<T>(); }
    virtual void write(Bitstream& out, const QVariant& value) const { out << value.value<T>(); }
    virtual QVariant read(Bitstream& in) const { T value; in >> value; return QVariant::fromValue(value); }
    virtual void writeDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const {
        out.writeDelta(value.value<T>(), reference.value<T>()); }
    virtual void readDelta(Bitstream& in, QVariant& value, const QVariant& reference) const {
        T rawValue; in.readDelta(rawValue, reference.value<T>()); value = QVariant::fromValue(rawValue); }
    virtual void writeRawDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const {
        out.writeRawDelta(value.value<T>(), reference.value<T>()); }
    virtual void readRawDelta(Bitstream& in, QVariant& value, const QVariant& reference) const {
        T rawValue; in.readRawDelta(rawValue, reference.value<T>()); value = QVariant::fromValue(rawValue); }
};

/// A streamer class for enumerated types.
class EnumTypeStreamer : public TypeStreamer {
public:
    
    EnumTypeStreamer(const QMetaObject* metaObject, const char* name);
    EnumTypeStreamer(const QMetaEnum& metaEnum);

    virtual const char* getName() const;
    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const;
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual Category getCategory() const;
    virtual int getBits() const;
    virtual QMetaEnum getMetaEnum() const;
    virtual bool equal(const QVariant& first, const QVariant& second) const;
    virtual void write(Bitstream& out, const QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual void writeDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const;
    virtual void readDelta(Bitstream& in, QVariant& value, const QVariant& reference) const;
    virtual void writeRawDelta(Bitstream& out, const QVariant& value, const QVariant& reference) const;
    virtual void readRawDelta(Bitstream& in, QVariant& value, const QVariant& reference) const;
    virtual void setEnumValue(QVariant& object, int value, const QHash<int, int>& mappings) const;

private:
    
    const QMetaObject* _metaObject;
    const char* _enumName;
    QByteArray _name;
    QMetaEnum _metaEnum;
    int _bits;
};

/// A streamer class for enums that maps to a local type.
class MappedEnumTypeStreamer : public TypeStreamer {
public:

    MappedEnumTypeStreamer(const TypeStreamer* baseStreamer, int bits, const QHash<int, int>& mappings);
    
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual void readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const;
    
private:
    
    const TypeStreamer* _baseStreamer;
    int _bits;
    QHash<int, int> _mappings;
};

/// Base class for generic type streamers, which contain all the metadata required to write out a type.
class GenericTypeStreamer : public TypeStreamer {
public:

    GenericTypeStreamer(const QByteArray& name);

    virtual const char* getName() const;
    virtual void putJSONVariantData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual QVariant readVariant(Bitstream& in) const;
    
protected:
    
    friend class Bitstream;
    friend class JSONReader;
    
    QByteArray _name;
    WeakTypeStreamerPointer _weakSelf; ///< promoted to strong references in GenericValue when reading
};

/// A streamer for generic enums.
class GenericEnumTypeStreamer : public GenericTypeStreamer {
public:

    GenericEnumTypeStreamer(const QByteArray& name, const QVector<NameIntPair>& values, int bits, const QByteArray& hash);

    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const;
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual void write(Bitstream& out, const QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual Category getCategory() const;
    
private:
    
    QVector<NameIntPair> _values;
    int _bits;
    QByteArray _hash;
};

/// A streamer for types compiled by mtc.
template<class T> class StreamableTypeStreamer : public SimpleTypeStreamer<T> {
public:
    
    virtual TypeStreamer::Category getCategory() const { return TypeStreamer::STREAMABLE_CATEGORY; }
    virtual const QVector<MetaField>& getMetaFields() const { return T::getMetaFields(); }
    virtual int getFieldIndex(const QByteArray& name) const { return T::getFieldIndex(name); }
    virtual void setField(QVariant& object, int index, const QVariant& value) const {
        static_cast<T*>(object.data())->setField(index, value); }
    virtual QVariant getField(const QVariant& object, int index) const {
        return static_cast<const T*>(object.constData())->getField(index); }
};

typedef QPair<TypeStreamerPointer, int> StreamerIndexPair;

/// A streamer class for streamables that maps to a local type.
class MappedStreamableTypeStreamer : public TypeStreamer {
public:

    MappedStreamableTypeStreamer(const TypeStreamer* baseStreamer, const QVector<StreamerIndexPair>& fields);
    
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual void readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const;
    
private:
    
    const TypeStreamer* _baseStreamer;
    QVector<StreamerIndexPair> _fields;
};

/// A streamer for generic enums.
class GenericStreamableTypeStreamer : public GenericTypeStreamer {
public:

    GenericStreamableTypeStreamer(const QByteArray& name, const QVector<StreamerNamePair>& fields, const QByteArray& hash);

    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const;
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual void write(Bitstream& out, const QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual Category getCategory() const;
    
private:
    
    QVector<StreamerNamePair> _fields;
    QByteArray _hash;
};

/// Base template for collection streamers.
template<class T> class CollectionTypeStreamer : public SimpleTypeStreamer<T> {
};

/// A streamer for list types.
template<class T> class CollectionTypeStreamer<QList<T> > : public SimpleTypeStreamer<QList<T> > {
public:
    
    virtual void writeMetadata(Bitstream& out, bool full) const { out << getValueStreamer(); }
    virtual TypeStreamer::Category getCategory() const { return TypeStreamer::LIST_CATEGORY; }
    virtual const TypeStreamer* getValueStreamer() const { return Bitstream::getTypeStreamer(qMetaTypeId<T>()); }
    virtual void insert(QVariant& object, const QVariant& value) const {
        static_cast<QList<T>*>(object.data())->append(value.value<T>()); }
    virtual void prune(QVariant& object, int size) const {
        QList<T>* list = static_cast<QList<T>*>(object.data()); list->erase(list->begin() + size, list->end()); }
    virtual QVariant getValue(const QVariant& object, int index) const {
        return QVariant::fromValue(static_cast<const QList<T>*>(object.constData())->at(index)); }
    virtual void setValue(QVariant& object, int index, const QVariant& value) const {
        static_cast<QList<T>*>(object.data())->replace(index, value.value<T>()); }
};

/// A streamer for vector types.
template<class T> class CollectionTypeStreamer<QVector<T> > : public SimpleTypeStreamer<QVector<T> > {
public:
    
    virtual void writeMetadata(Bitstream& out, bool full) const { out << getValueStreamer(); }
    virtual TypeStreamer::Category getCategory() const { return TypeStreamer::LIST_CATEGORY; }
    virtual const TypeStreamer* getValueStreamer() const { return Bitstream::getTypeStreamer(qMetaTypeId<T>()); }
    virtual void insert(QVariant& object, const QVariant& value) const {
        static_cast<QVector<T>*>(object.data())->append(value.value<T>()); }
    virtual void prune(QVariant& object, int size) const {
        QVector<T>* list = static_cast<QVector<T>*>(object.data()); list->erase(list->begin() + size, list->end()); }
    virtual QVariant getValue(const QVariant& object, int index) const {
        return QVariant::fromValue(static_cast<const QVector<T>*>(object.constData())->at(index)); }
    virtual void setValue(QVariant& object, int index, const QVariant& value) const {
        static_cast<QVector<T>*>(object.data())->replace(index, value.value<T>()); }
};

/// A streamer for lists that maps to a local type.
class MappedListTypeStreamer : public TypeStreamer {
public:
    
    MappedListTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& valueStreamer);
    
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual void readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const;
    
protected:
    
    const TypeStreamer* _baseStreamer;
    TypeStreamerPointer _valueStreamer;
};

/// A streamer for generic lists.
class GenericListTypeStreamer : public GenericTypeStreamer {
public:

    GenericListTypeStreamer(const QByteArray& name, const TypeStreamerPointer& valueStreamer);
    
    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const;
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual void write(Bitstream& out, const QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual Category getCategory() const;
    
protected:
    
    TypeStreamerPointer _valueStreamer;
};

/// A streamer for set types.
template<class T> class CollectionTypeStreamer<QSet<T> > : public SimpleTypeStreamer<QSet<T> > {
public:
    
    virtual void writeMetadata(Bitstream& out, bool full) const { out << getValueStreamer(); }
    virtual TypeStreamer::Category getCategory() const { return TypeStreamer::SET_CATEGORY; }
    virtual const TypeStreamer* getValueStreamer() const { return Bitstream::getTypeStreamer(qMetaTypeId<T>()); }
    virtual void insert(QVariant& object, const QVariant& value) const {
        static_cast<QSet<T>*>(object.data())->insert(value.value<T>()); }
    virtual bool remove(QVariant& object, const QVariant& key) const {
        return static_cast<QSet<T>*>(object.data())->remove(key.value<T>()); }
};

/// A streamer for sets that maps to a local type.
class MappedSetTypeStreamer : public MappedListTypeStreamer {
public:
    
    MappedSetTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& valueStreamer);
    
    virtual void readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const;
};

/// A streamer for generic sets.
class GenericSetTypeStreamer : public GenericListTypeStreamer {
public:

    GenericSetTypeStreamer(const QByteArray& name, const TypeStreamerPointer& valueStreamer);
    
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual Category getCategory() const;
};

/// A streamer for hash types.
template<class K, class V> class CollectionTypeStreamer<QHash<K, V> > : public SimpleTypeStreamer<QHash<K, V> > {
public:
    
    virtual void writeMetadata(Bitstream& out, bool full) const { out << getKeyStreamer() << getValueStreamer(); }
    virtual TypeStreamer::Category getCategory() const { return TypeStreamer::MAP_CATEGORY; }
    virtual const TypeStreamer* getKeyStreamer() const { return Bitstream::getTypeStreamer(qMetaTypeId<K>()); }
    virtual const TypeStreamer* getValueStreamer() const { return Bitstream::getTypeStreamer(qMetaTypeId<V>()); }
    virtual void insert(QVariant& object, const QVariant& key, const QVariant& value) const {
        static_cast<QHash<K, V>*>(object.data())->insert(key.value<K>(), value.value<V>()); }
    virtual bool remove(QVariant& object, const QVariant& key) const {
        return static_cast<QHash<K, V>*>(object.data())->remove(key.value<K>()); }
    virtual QVariant getValue(const QVariant& object, const QVariant& key) const {
        return QVariant::fromValue(static_cast<const QHash<K, V>*>(object.constData())->value(key.value<K>())); }
};

/// A streamer for maps that maps to a local type.
class MappedMapTypeStreamer : public TypeStreamer {
public:
    
    MappedMapTypeStreamer(const TypeStreamer* baseStreamer, const TypeStreamerPointer& keyStreamer,
        const TypeStreamerPointer& valueStreamer);
    
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual void readRawDelta(Bitstream& in, QVariant& object, const QVariant& reference) const;
    
private:
    
    const TypeStreamer* _baseStreamer;
    TypeStreamerPointer _keyStreamer;
    TypeStreamerPointer _valueStreamer;
};

/// A streamer for generic maps.
class GenericMapTypeStreamer : public GenericTypeStreamer {
public:

    GenericMapTypeStreamer(const QByteArray& name, const TypeStreamerPointer& keyStreamer,
        const TypeStreamerPointer& valueStreamer);
    
    virtual void writeMetadata(Bitstream& out, bool full) const;
    virtual QJsonValue getJSONMetadata(JSONWriter& writer) const;
    virtual QJsonValue getJSONData(JSONWriter& writer, const QVariant& value) const;
    virtual void putJSONData(JSONReader& reader, const QJsonValue& data, QVariant& value) const;
    virtual void write(Bitstream& out, const QVariant& value) const;
    virtual QVariant read(Bitstream& in) const;
    virtual Category getCategory() const;
    
private:

    TypeStreamerPointer _keyStreamer;
    TypeStreamerPointer _valueStreamer;
};

/// A streamer class for generic values.
class GenericValueStreamer : public SimpleTypeStreamer<GenericValue> {
public:
    
    QJsonValue getJSONVariantData(JSONWriter& writer, const QVariant& value) const;
    virtual void writeVariant(Bitstream& out, const QVariant& value) const;
};

/// Macro for registering simple type streamers.  Typically, one would use this at the top level of the source file
/// associated with the type.
#define REGISTER_SIMPLE_TYPE_STREAMER(X) static int X##Streamer = \
    Bitstream::registerTypeStreamer(qMetaTypeId<X>(), new SimpleTypeStreamer<X>());

/// Macro for registering collection type (QList, QVector, QSet, QMap) streamers.  Typically, one would use this at the top
/// level of the source file associated with the type.
#define REGISTER_COLLECTION_TYPE_STREAMER(X) static int x##Streamer = \
    Bitstream::registerTypeStreamer(qMetaTypeId<X>(), new CollectionTypeStreamer<X>());

/// Declares the metatype and the streaming operators.  Typically, one would use this immediately after the definition of a
/// type flagged as STREAMABLE in its header file.  The type should have a no-argument constructor.  The last lines of this
/// macro ensure that the generated file will be included in the link phase.
#ifdef _WIN32
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    Bitstream& operator<<(Bitstream& out, const X& obj); \
    Bitstream& operator>>(Bitstream& in, X& obj); \
    template<> void Bitstream::writeRawDelta(const X& value, const X& reference); \
    template<> void Bitstream::readRawDelta(X& value, const X& reference); \
    template<> QJsonValue JSONWriter::getData(const X& value); \
    template<> void JSONReader::putData(const QJsonValue& data, X& value); \
    bool operator==(const X& first, const X& second); \
    bool operator!=(const X& first, const X& second); \
    static const int* _TypePtr##X = &X::Type;
#elif __GNUC__
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    Bitstream& operator<<(Bitstream& out, const X& obj); \
    Bitstream& operator>>(Bitstream& in, X& obj); \
    template<> void Bitstream::writeRawDelta(const X& value, const X& reference); \
    template<> void Bitstream::readRawDelta(X& value, const X& reference); \
    template<> QJsonValue JSONWriter::getData(const X& value); \
    template<> void JSONReader::putData(const QJsonValue& data, X& value); \
    bool operator==(const X& first, const X& second); \
    bool operator!=(const X& first, const X& second); \
    __attribute__((unused)) static const int* _TypePtr##X = &X::Type;
#else
#define STRINGIFY(x) #x
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    Bitstream& operator<<(Bitstream& out, const X& obj); \
    Bitstream& operator>>(Bitstream& in, X& obj); \
    template<> void Bitstream::writeRawDelta(const X& value, const X& reference); \
    template<> void Bitstream::readRawDelta(X& value, const X& reference); \
    template<> QJsonValue JSONWriter::getData(const X& value); \
    template<> void JSONReader::putData(const QJsonValue& data, X& value); \
    bool operator==(const X& first, const X& second); \
    bool operator!=(const X& first, const X& second); \
    static const int* _TypePtr##X = &X::Type; \
    _Pragma(STRINGIFY(unused(_TypePtr##X)))
#endif

/// Declares an enum metatype.  This is used when one desires to use an enum defined in a QObject outside of that class's
/// direct properties.  Typically, one would use this immediately after the definition of the QObject containing the enum
/// in its header file.
#define DECLARE_ENUM_METATYPE(S, N) Q_DECLARE_METATYPE(S::N) \
    Bitstream& operator<<(Bitstream& out, const S::N& obj); \
    Bitstream& operator>>(Bitstream& in, S::N& obj); \
    template<> inline void Bitstream::writeRawDelta(const S::N& value, const S::N& reference) { *this << value; } \
    template<> inline void Bitstream::readRawDelta(S::N& value, const S::N& reference) { *this >> value; } \
    template<> inline QJsonValue JSONWriter::getData(const S::N& value) { return (int)value; } \
    template<> inline void JSONReader::putData(const QJsonValue& data, S::N& value) { value = (S::N)data.toInt(); }

/// Implements an enum metatype.  This performs the implementation of the previous macro, and would normally be used at the
/// top level of the source file associated with the QObject containing the enum.
#define IMPLEMENT_ENUM_METATYPE(S, N) \
    static int S##N##MetaTypeId = registerEnumMetaType<S::N>(&S::staticMetaObject, #N); \
    Bitstream& operator<<(Bitstream& out, const S::N& obj) { \
        static int bits = Bitstream::getTypeStreamer(qMetaTypeId<S::N>())->getBits(); \
        return out.write(&obj, bits); \
    } \
    Bitstream& operator>>(Bitstream& in, S::N& obj) { \
        static int bits = Bitstream::getTypeStreamer(qMetaTypeId<S::N>())->getBits(); \
        obj = (S::N)0; \
        return in.read(&obj, bits); \
    }
    
/// Registers a simple type and its streamer.  This would typically be used at the top level of the source file associated with
/// the type, and combines the registration of the type with the Qt metatype system with the registration of a simple type
/// streamer.
/// \return the metatype id
template<class T> int registerSimpleMetaType() {
    int type = qRegisterMetaType<T>();
    Bitstream::registerTypeStreamer(type, new SimpleTypeStreamer<T>());
    return type;
}

/// Registers an enum type and its streamer.  Rather than using this directly, consider using the IMPLEMENT_ENUM_METATYPE
/// macro.
/// \return the metatype id
template<class T> int registerEnumMetaType(const QMetaObject* metaObject, const char* name) {
    int type = qRegisterMetaType<T>();
    Bitstream::registerTypeStreamer(type, new EnumTypeStreamer(metaObject, name));
    return type;
}

/// Registers a streamable type and its streamer.  Rather than using this directly, consider using the
/// DECLARE_STREAMABLE_METATYPE macro and the mtc code generation tool.
/// \return the metatype id
template<class T> int registerStreamableMetaType() {
    int type = qRegisterMetaType<T>();
    Bitstream::registerTypeStreamer(type, new StreamableTypeStreamer<T>());
    return type;
}

/// Registers a collection type and its streamer.  This would typically be used at the top level of the source file associated
/// with the type, and combines the registration of the type with the Qt metatype system with the registration of a collection
/// type streamer.
/// \return the metatype id
template<class T> int registerCollectionMetaType() {
    int type = qRegisterMetaType<T>();
    Bitstream::registerTypeStreamer(type, new CollectionTypeStreamer<T>());
    return type;
}

/// Flags a class as streamable.  Use as you would Q_OBJECT: as the first line of a class definition, before any members or
/// access qualifiers.  Typically, one would follow the definition with DECLARE_STREAMABLE_METATYPE.  The mtc tool looks for
/// this macro in order to generate streaming (etc.) code for types.
#define STREAMABLE public: \
    static const int Type; \
    static const QVector<MetaField>& getMetaFields(); \
    static int getFieldIndex(const QByteArray& name); \
    void setField(int index, const QVariant& value); \
    QVariant getField(int index) const; \
    private: \
    static QHash<QByteArray, int> createFieldIndices();

/// Flags a (public) field within or base class of a streamable type as streaming.  Use before all base classes or fields that
/// you want to stream.
#define STREAM

#endif // hifi_Bitstream_h
