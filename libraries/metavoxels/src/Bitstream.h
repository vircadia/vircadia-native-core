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
#include <QSharedPointer>
#include <QVariant>

class QByteArray;
class QDataStream;
class QMetaObject;
class QObject;

class Attribute;
class Bitstream;
class TypeStreamer;

typedef QSharedPointer<Attribute> AttributePointer;

/// Streams integer identifiers that conform to the following pattern: each ID encountered in the stream is either one that
/// has been sent (received) before, or is one more than the highest previously encountered ID (starting at zero).  This allows
/// us to use the minimum number of bits to encode the IDs.
class IDStreamer {
public:
    
    IDStreamer(Bitstream& stream);
    
    IDStreamer& operator<<(int value);
    IDStreamer& operator>>(int& value);
    
private:
    
    Bitstream& _stream;
    int _bits;
};

/// Provides a means to stream repeated values efficiently.  The value is first streamed along with a unique ID.  When
/// subsequently streamed, only the ID is sent.
template<class T> class RepeatedValueStreamer {
public:
    
    RepeatedValueStreamer(Bitstream& stream) : _stream(stream), _idStreamer(stream), _lastNewID(0) { }
    
    RepeatedValueStreamer& operator<<(T value);
    RepeatedValueStreamer& operator>>(T& value);
    
private:
    
    Bitstream& _stream;
    IDStreamer _idStreamer;
    int _lastNewID;
    QHash<T, int> _ids;
    QHash<int, T> _values;
};

template<class T> inline RepeatedValueStreamer<T>& RepeatedValueStreamer<T>::operator<<(T value) {
    int& id = _ids[value];
    if (id == 0) {
        _idStreamer << (id = ++_lastNewID);    
        _stream << value;
        
    } else {
        _idStreamer << id;
    }
    return *this;
}

template<class T> inline RepeatedValueStreamer<T>& RepeatedValueStreamer<T>::operator>>(T& value) {
    int id;
    _idStreamer >> id;
    typename QHash<int, T>::iterator it = _values.find(id);
    if (it == _values.end()) {
        _stream >> value;
        _values.insert(id, value);     
        
    } else {
        value = *it;
    }
    return *this;
}

/// A stream for bit-aligned data.
class Bitstream {
public:

    /// Registers a metaobject under its name so that instances of it can be streamed.
    /// \return zero; the function only returns a value so that it can be used in static initialization
    static int registerMetaObject(const char* className, const QMetaObject* metaObject);

    /// Registers a streamer for the specified Qt-registered type.
    /// \return zero; the function only returns a value so that it can be used in static initialization
    static int registerTypeStreamer(int type, TypeStreamer* streamer);

    /// Creates a new bitstream.  Note: the stream may be used for reading or writing, but not both.
    Bitstream(QDataStream& underlying);

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

    /// Returns a reference to the attribute streamer.
    RepeatedValueStreamer<AttributePointer>& getAttributeStreamer() { return _attributeStreamer; }

    Bitstream& operator<<(bool value);
    Bitstream& operator>>(bool& value);
    
    Bitstream& operator<<(int value);
    Bitstream& operator>>(int& value);
    
    Bitstream& operator<<(const QByteArray& string);
    Bitstream& operator>>(QByteArray& string);
    
    Bitstream& operator<<(const QString& string);
    Bitstream& operator>>(QString& string);
    
    Bitstream& operator<<(const QVariant& value);
    Bitstream& operator>>(QVariant& value);
    
    Bitstream& operator<<(QObject* object);
    Bitstream& operator>>(QObject*& object);
    
    Bitstream& operator<<(const AttributePointer& attribute);
    Bitstream& operator>>(AttributePointer& attribute);
    
private:
   
    QDataStream& _underlying;
    quint8 _byte;
    int _position;

    RepeatedValueStreamer<QByteArray> _classNameStreamer;
    RepeatedValueStreamer<AttributePointer> _attributeStreamer;

    static QHash<QByteArray, const QMetaObject*>& getMetaObjects();
    static QHash<int, TypeStreamer*>& getTypeStreamers();
};

/// Macro for registering streamable meta-objects.
#define REGISTER_META_OBJECT(x) static int x##Registration = Bitstream::registerMetaObject(#x, &x::staticMetaObject);

/// Interface for objects that can write values to and read values from bitstreams.
class TypeStreamer {
public:
    
    virtual void write(Bitstream& out, const QVariant& value) const = 0;
    virtual QVariant read(Bitstream& in) const = 0;
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

#endif /* defined(__interface__Bitstream__) */
