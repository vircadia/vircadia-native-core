//
//  Bitstream.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Bitstream__
#define __interface__Bitstream__

#include <QHash>
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
class QUrl;

class Attribute;
class AttributeValue;
class Bitstream;
class FieldReader;
class ObjectReader;
class OwnedAttributeValue;
class PropertyReader;
class TypeReader;
class TypeStreamer;

typedef SharedObjectPointerTemplate<Attribute> AttributePointer;

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
    
    int takePersistentID(P value) { return _persistentIDs.take(value); }
    
    V takePersistentValue(int id) { V value = _persistentValues.take(id); _valueIDs.remove(value); return value; }
    
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

/// A stream for bit-aligned data.
class Bitstream : public QObject {
    Q_OBJECT

public:

    class WriteMappings {
    public:
        QHash<const QMetaObject*, int> metaObjectOffsets;
        QHash<const TypeStreamer*, int> typeStreamerOffsets;
        QHash<AttributePointer, int> attributeOffsets;
        QHash<QScriptString, int> scriptStringOffsets;
        QHash<SharedObjectPointer, int> sharedObjectOffsets;
    };

    class ReadMappings {
    public:
        QHash<int, ObjectReader> metaObjectValues;
        QHash<int, TypeReader> typeStreamerValues;
        QHash<int, AttributePointer> attributeValues;
        QHash<int, QScriptString> scriptStringValues;
        QHash<int, SharedObjectPointer> sharedObjectValues;
    };

    /// Registers a metaobject under its name so that instances of it can be streamed.
    /// \return zero; the function only returns a value so that it can be used in static initialization
    static int registerMetaObject(const char* className, const QMetaObject* metaObject);

    /// Registers a streamer for the specified Qt-registered type.
    /// \return zero; the function only returns a value so that it can be used in static initialization
    static int registerTypeStreamer(int type, TypeStreamer* streamer);

    /// Returns the streamer registered for the supplied type, if any.
    static const TypeStreamer* getTypeStreamer(int type);

    /// Returns the meta-object registered under the supplied class name, if any.
    static const QMetaObject* getMetaObject(const QByteArray& className);

    /// Returns the list of registered subclasses for the supplied meta-object.
    static QList<const QMetaObject*> getMetaObjectSubClasses(const QMetaObject* metaObject);

    enum MetadataType { NO_METADATA, HASH_METADATA, FULL_METADATA };

    /// Creates a new bitstream.  Note: the stream may be used for reading or writing, but not both.
    Bitstream(QDataStream& underlying, MetadataType metadataType = NO_METADATA, QObject* parent = NULL);

    /// Substitutes the supplied metaobject for the given class name's default mapping.
    void addMetaObjectSubstitution(const QByteArray& className, const QMetaObject* metaObject);
    
    /// Substitutes the supplied type for the given type name's default mapping.
    void addTypeSubstitution(const QByteArray& typeName, int type);

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

    /// Returns a reference to the weak hash storing shared objects for this stream.
    const WeakSharedObjectHash& getWeakSharedObjectHash() const { return _weakSharedObjectHash; }

    /// Removes a shared object from the read mappings.
    void clearSharedObject(int id);

    Bitstream& operator<<(bool value);
    Bitstream& operator>>(bool& value);
    
    Bitstream& operator<<(int value);
    Bitstream& operator>>(int& value);
    
    Bitstream& operator<<(uint value);
    Bitstream& operator>>(uint& value);
    
    Bitstream& operator<<(float value);
    Bitstream& operator>>(float& value);
    
    Bitstream& operator<<(const glm::vec3& value);
    Bitstream& operator>>(glm::vec3& value);
    
    Bitstream& operator<<(const QByteArray& string);
    Bitstream& operator>>(QByteArray& string);
    
    Bitstream& operator<<(const QColor& color);
    Bitstream& operator>>(QColor& color);
    
    Bitstream& operator<<(const QString& string);
    Bitstream& operator>>(QString& string);
    
    Bitstream& operator<<(const QUrl& url);
    Bitstream& operator>>(QUrl& url);
    
    Bitstream& operator<<(const QVariant& value);
    Bitstream& operator>>(QVariant& value);
    
    Bitstream& operator<<(const AttributeValue& attributeValue);
    Bitstream& operator>>(OwnedAttributeValue& attributeValue);
    
    template<class T> Bitstream& operator<<(const QList<T>& list);
    template<class T> Bitstream& operator>>(QList<T>& list);
    
    template<class T> Bitstream& operator<<(const QSet<T>& set);
    template<class T> Bitstream& operator>>(QSet<T>& set);
    
    template<class K, class V> Bitstream& operator<<(const QHash<K, V>& hash);
    template<class K, class V> Bitstream& operator>>(QHash<K, V>& hash);
    
    Bitstream& operator<<(const QObject* object);
    Bitstream& operator>>(QObject*& object);
    
    Bitstream& operator<<(const QMetaObject* metaObject);
    Bitstream& operator>>(const QMetaObject*& metaObject);
    Bitstream& operator>>(ObjectReader& objectReader);
    
    Bitstream& operator<<(const TypeStreamer* streamer);
    Bitstream& operator>>(const TypeStreamer*& streamer);
    Bitstream& operator>>(TypeReader& reader);
    
    Bitstream& operator<<(const AttributePointer& attribute);
    Bitstream& operator>>(AttributePointer& attribute);
    
    Bitstream& operator<<(const QScriptString& string);
    Bitstream& operator>>(QScriptString& string);
    
    Bitstream& operator<<(const SharedObjectPointer& object);
    Bitstream& operator>>(SharedObjectPointer& object);
    
    Bitstream& operator<(const QMetaObject* metaObject);
    Bitstream& operator>(ObjectReader& objectReader);
    
    Bitstream& operator<(const TypeStreamer* streamer);
    Bitstream& operator>(TypeReader& reader);
    
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
    
    QDataStream& _underlying;
    quint8 _byte;
    int _position;

    MetadataType _metadataType;

    RepeatedValueStreamer<const QMetaObject*, const QMetaObject*, ObjectReader> _metaObjectStreamer;
    RepeatedValueStreamer<const TypeStreamer*, const TypeStreamer*, TypeReader> _typeStreamerStreamer;
    RepeatedValueStreamer<AttributePointer> _attributeStreamer;
    RepeatedValueStreamer<QScriptString> _scriptStringStreamer;
    RepeatedValueStreamer<SharedObjectPointer, SharedObject*> _sharedObjectStreamer;

    WeakSharedObjectHash _weakSharedObjectHash;

    QHash<QByteArray, const QMetaObject*> _metaObjectSubstitutions;
    QHash<QByteArray, const TypeStreamer*> _typeStreamerSubstitutions;

    static QHash<QByteArray, const QMetaObject*>& getMetaObjects();
    static QMultiHash<const QMetaObject*, const QMetaObject*>& getMetaObjectSubClasses();
    static QHash<int, const TypeStreamer*>& getTypeStreamers();
    static QVector<PropertyReader> getPropertyReaders(const QMetaObject* metaObject);
};

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

/// Contains the information required to read a type from the stream.
class TypeReader {
public:

    TypeReader(const QByteArray& typeName = QByteArray(), const TypeStreamer* streamer = NULL,
        bool exactMatch = true, const QVector<FieldReader>& fields = QVector<FieldReader>());

    const QByteArray& getTypeName() const { return _typeName; }
    const TypeStreamer* getStreamer() const { return _streamer; }

    QVariant read(Bitstream& in) const;

    bool matchesExactly(const TypeStreamer* streamer) const;

    bool operator==(const TypeReader& other) const { return _typeName == other._typeName; }
    bool operator!=(const TypeReader& other) const { return _typeName != other._typeName; }
    
private:
    
    QByteArray _typeName;
    const TypeStreamer* _streamer;
    bool _exactMatch;
    QVector<FieldReader> _fields;
};

uint qHash(const TypeReader& typeReader, uint seed = 0);

/// Contains the information required to read a metatype field from the stream and apply it.
class FieldReader {
public:
    
    FieldReader(const TypeReader& reader = TypeReader(), int index = -1);

    const TypeReader& getReader() const { return _reader; }
    int getIndex() const { return _index; }

    void read(Bitstream& in, const TypeStreamer* streamer, QVariant& object) const;

private:
    
    TypeReader _reader;
    int _index;
};

/// Contains the information required to read an object from the stream.
class ObjectReader {
public:

    ObjectReader(const QByteArray& className = QByteArray(), const QMetaObject* metaObject = NULL,
        const QVector<PropertyReader>& properties = QVector<PropertyReader>());

    const QByteArray& getClassName() const { return _className; }
    const QMetaObject* getMetaObject() const { return _metaObject; }

    QObject* read(Bitstream& in, QObject* object = NULL) const;

    bool operator==(const ObjectReader& other) const { return _className == other._className; }
    bool operator!=(const ObjectReader& other) const { return _className != other._className; }

private:

    QByteArray _className;
    const QMetaObject* _metaObject;
    QVector<PropertyReader> _properties;
};

uint qHash(const ObjectReader& objectReader, uint seed = 0);

/// Contains the information required to read an object property from the stream and apply it.
class PropertyReader {
public:

    PropertyReader(const TypeReader& reader = TypeReader(), const QMetaProperty& property = QMetaProperty());

    const TypeReader& getReader() const { return _reader; }

    void read(Bitstream& in, QObject* object) const;

private:

    TypeReader _reader;
    QMetaProperty _property;
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

/// Macro for registering streamable meta-objects.
#define REGISTER_META_OBJECT(x) static int x##Registration = Bitstream::registerMetaObject(#x, &x::staticMetaObject);

/// Interface for objects that can write values to and read values from bitstreams.
class TypeStreamer {
public:
    
    virtual ~TypeStreamer();
    
    void setType(int type) { _type = type; }
    int getType() const { return _type; }
    
    virtual void write(Bitstream& out, const QVariant& value) const = 0;
    virtual QVariant read(Bitstream& in) const = 0;

    virtual const QVector<MetaField>& getMetaFields() const;
    virtual int getFieldIndex(const QByteArray& name) const;
    virtual void setField(int index, QVariant& object, const QVariant& value) const;

private:
    
    int _type;
};

/// A streamer that works with Bitstream's operators.
template<class T> class SimpleTypeStreamer : public TypeStreamer {
public:
    
    virtual void write(Bitstream& out, const QVariant& value) const { out << value.value<T>(); }
    virtual QVariant read(Bitstream& in) const { T value; in >> value; return QVariant::fromValue(value); }
};

/// A streamer that works with Bitstream's operators.
template<class T> class CompoundTypeStreamer : public SimpleTypeStreamer<T> {
public:
    
    virtual const QVector<MetaField>& getMetaFields() const { return T::getMetaFields(); }
    virtual int getFieldIndex(const QByteArray& name) const { return T::getFieldIndex(name); }
    virtual void setField(int index, QVariant& object, const QVariant& value) const {
        static_cast<T*>(object.data())->setField(index, value); }
};

/// Macro for registering simple type streamers.
#define REGISTER_SIMPLE_TYPE_STREAMER(x) static int x##Streamer = \
    Bitstream::registerTypeStreamer(qMetaTypeId<x>(), new SimpleTypeStreamer<x>());

/// Declares the metatype and the streaming operators.  The last lines
/// ensure that the generated file will be included in the link phase. 
#ifdef _WIN32
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    Bitstream& operator<<(Bitstream& out, const X& obj); \
    Bitstream& operator>>(Bitstream& in, X& obj); \
    bool operator==(const X& first, const X& second); \
    bool operator!=(const X& first, const X& second); \
    static const int* _TypePtr##X = &X::Type;
#elif __GNUC__
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    Bitstream& operator<<(Bitstream& out, const X& obj); \
    Bitstream& operator>>(Bitstream& in, X& obj); \
    bool operator==(const X& first, const X& second); \
    bool operator!=(const X& first, const X& second); \
    __attribute__((unused)) static const int* _TypePtr##X = &X::Type;
#else
#define STRINGIFY(x) #x
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    Bitstream& operator<<(Bitstream& out, const X& obj); \
    Bitstream& operator>>(Bitstream& in, X& obj); \
    bool operator==(const X& first, const X& second); \
    bool operator!=(const X& first, const X& second); \
    static const int* _TypePtr##X = &X::Type; \
    _Pragma(STRINGIFY(unused(_TypePtr##X)))
#endif

/// Registers a streamable type and its streamer.
template<class T> int registerStreamableMetaType() {
    int type = qRegisterMetaType<T>();
    Bitstream::registerTypeStreamer(type, new CompoundTypeStreamer<T>());
    return type;
}

/// Flags a class as streamable (use as you would Q_OBJECT).
#define STREAMABLE public: \
    static const int Type; \
    static const QVector<MetaField>& getMetaFields(); \
    static int getFieldIndex(const QByteArray& name); \
    void setField(int index, const QVariant& value); \
    private: \
    static QHash<QByteArray, int> createFieldIndices();

/// Flags a field or base class as streaming.
#define STREAM

#endif /* defined(__interface__Bitstream__) */
