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
#include <QMetaType>
#include <QPointer>
#include <QScriptString>
#include <QSharedPointer>
#include <QVariant>
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
class OwnedAttributeValue;
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
template<class T, class P = T> class RepeatedValueStreamer {
public:
    
    RepeatedValueStreamer(Bitstream& stream) : _stream(stream), _idStreamer(stream),
        _lastPersistentID(0), _lastTransientOffset(0) { }
    
    QHash<T, int> getAndResetTransientOffsets();
    
    void persistTransientOffsets(const QHash<T, int>& transientOffsets);
    
    QHash<int, T> getAndResetTransientValues();
    
    void persistTransientValues(const QHash<int, T>& transientValues);
    
    int takePersistentID(P value) { return _persistentIDs.take(value); }
    
    void removePersistentValue(int id) { _persistentIDs.remove(_persistentValues.take(id)); }
    
    RepeatedValueStreamer& operator<<(T value);
    RepeatedValueStreamer& operator>>(T& value);
    
private:
    
    Bitstream& _stream;
    IDStreamer _idStreamer;
    int _lastPersistentID;
    int _lastTransientOffset;
    QHash<P, int> _persistentIDs;
    QHash<T, int> _transientOffsets;
    QHash<int, T> _persistentValues;
    QHash<int, T> _transientValues;
};

template<class T, class P> inline QHash<T, int> RepeatedValueStreamer<T, P>::getAndResetTransientOffsets() {
    QHash<T, int> transientOffsets;
    _transientOffsets.swap(transientOffsets);
    _lastTransientOffset = 0;
    _idStreamer.setBitsFromValue(_lastPersistentID);
    return transientOffsets;
}

template<class T, class P> inline void RepeatedValueStreamer<T, P>::persistTransientOffsets(
        const QHash<T, int>& transientOffsets) {
    int oldLastPersistentID = _lastPersistentID;
    for (typename QHash<T, int>::const_iterator it = transientOffsets.constBegin(); it != transientOffsets.constEnd(); it++) {
        int& id = _persistentIDs[it.key()];
        if (id == 0) {
            id = oldLastPersistentID + it.value();
            _lastPersistentID = qMax(_lastPersistentID, id);
        }
    }
    _idStreamer.setBitsFromValue(_lastPersistentID);
}

template<class T, class P> inline QHash<int, T> RepeatedValueStreamer<T, P>::getAndResetTransientValues() {
    QHash<int, T> transientValues;
    _transientValues.swap(transientValues);
    _idStreamer.setBitsFromValue(_lastPersistentID);
    return transientValues;
}

template<class T, class P> inline void RepeatedValueStreamer<T, P>::persistTransientValues(
        const QHash<int, T>& transientValues) {
    int oldLastPersistentID = _lastPersistentID;
    for (typename QHash<int, T>::const_iterator it = transientValues.constBegin(); it != transientValues.constEnd(); it++) {
        int& id = _persistentIDs[it.value()];
        if (id == 0) {
            id = oldLastPersistentID + it.key();
            _lastPersistentID = qMax(_lastPersistentID, id);
            _persistentValues.insert(id, it.value());
        }
    }
    _idStreamer.setBitsFromValue(_lastPersistentID);
}

template<class T, class P> inline RepeatedValueStreamer<T, P>& RepeatedValueStreamer<T, P>::operator<<(T value) {
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

template<class T, class P> inline RepeatedValueStreamer<T, P>& RepeatedValueStreamer<T, P>::operator>>(T& value) {
    int id;
    _idStreamer >> id;
    if (id <= _lastPersistentID) {
        value = _persistentValues.value(id);
        
    } else {
        int offset = id - _lastPersistentID;
        typename QHash<int, T>::iterator it = _transientValues.find(offset);
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
        QHash<int, const QMetaObject*> metaObjectValues;
        QHash<int, const TypeStreamer*> typeStreamerValues;
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

    /// Returns the meta-object registered under the supplied class name, if any.
    static const QMetaObject* getMetaObject(const QByteArray& className);

    /// Returns the list of registered subclasses for the supplied meta-object.
    static QList<const QMetaObject*> getMetaObjectSubClasses(const QMetaObject* metaObject);

    /// Creates a new bitstream.  Note: the stream may be used for reading or writing, but not both.
    Bitstream(QDataStream& underlying, QObject* parent = NULL);

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

    /// Removes a shared object from the read mappings.
    void clearSharedObject(int id) { _sharedObjectStreamer.removePersistentValue(id); }

    Bitstream& operator<<(bool value);
    Bitstream& operator>>(bool& value);
    
    Bitstream& operator<<(int value);
    Bitstream& operator>>(int& value);
    
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
    
    Bitstream& operator<<(const TypeStreamer* streamer);
    Bitstream& operator>>(const TypeStreamer*& streamer);
    
    Bitstream& operator<<(const AttributePointer& attribute);
    Bitstream& operator>>(AttributePointer& attribute);
    
    Bitstream& operator<<(const QScriptString& string);
    Bitstream& operator>>(QScriptString& string);
    
    Bitstream& operator<<(const SharedObjectPointer& object);
    Bitstream& operator>>(SharedObjectPointer& object);
    
    Bitstream& operator<(const QMetaObject* metaObject);
    Bitstream& operator>(const QMetaObject*& metaObject);
    
    Bitstream& operator<(const TypeStreamer* streamer);
    Bitstream& operator>(const TypeStreamer*& streamer);
    
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
    
    void readProperties(QObject* object);
   
    QDataStream& _underlying;
    quint8 _byte;
    int _position;

    RepeatedValueStreamer<const QMetaObject*> _metaObjectStreamer;
    RepeatedValueStreamer<const TypeStreamer*> _typeStreamerStreamer;
    RepeatedValueStreamer<AttributePointer> _attributeStreamer;
    RepeatedValueStreamer<QScriptString> _scriptStringStreamer;
    RepeatedValueStreamer<SharedObjectPointer, SharedObject*> _sharedObjectStreamer;

    QHash<int, QPointer<SharedObject> > _transientSharedObjects;

    static QHash<QByteArray, const QMetaObject*>& getMetaObjects();
    static QMultiHash<const QMetaObject*, const QMetaObject*>& getMetaObjectSubClasses();
    static QHash<int, const TypeStreamer*>& getTypeStreamers();
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

Q_DECLARE_METATYPE(const QMetaObject*)

/// Macro for registering streamable meta-objects.
#define REGISTER_META_OBJECT(x) static int x##Registration = Bitstream::registerMetaObject(#x, &x::staticMetaObject);

/// Interface for objects that can write values to and read values from bitstreams.
class TypeStreamer {
public:
    
    void setType(int type) { _type = type; }
    int getType() const { return _type; }
    
    virtual void write(Bitstream& out, const QVariant& value) const = 0;
    virtual QVariant read(Bitstream& in) const = 0;

private:
    
    int _type;
};

/// A streamer that works with Bitstream's operators.
template<class T> class SimpleTypeStreamer : public TypeStreamer {
public:
    
    virtual void write(Bitstream& out, const QVariant& value) const { out << value.value<T>(); }
    virtual QVariant read(Bitstream& in) const { T value; in >> value; return QVariant::fromValue(value); }
};

/// Macro for registering simple type streamers.
#define REGISTER_SIMPLE_TYPE_STREAMER(x) static int x##Streamer = \
    Bitstream::registerTypeStreamer(QMetaType::type(#x), new SimpleTypeStreamer<x>());

#ifdef WIN32
#define _Pragma __pragma
#endif

/// Declares the metatype and the streaming operators.  The last lines
/// ensure that the generated file will be included in the link phase. 
#define STRINGIFY(x) #x
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
    Bitstream::registerTypeStreamer(type, new SimpleTypeStreamer<T>());
    return type;
}

/// Flags a class as streamable (use as you would Q_OBJECT).
#define STREAMABLE public: static const int Type; private:

/// Flags a field or base class as streaming.
#define STREAM

#endif /* defined(__interface__Bitstream__) */
