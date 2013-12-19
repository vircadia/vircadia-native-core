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
#include <QtGlobal>

class QByteArray;
class QDataStream;
class QMetaObject;

/// A stream for bit-aligned data.
class Bitstream {
public:

    /// Registers a metaobject under its name so that instances of it can be streamed.
    static void registerMetaObject(const QByteArray& name, const QMetaObject* metaObject);

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

    Bitstream& operator<<(bool value);
    Bitstream& operator>>(bool& value);
    
    Bitstream& operator<<(qint32 value);
    Bitstream& operator>>(qint32& value);
    
    
    
private:
   
    QDataStream& _underlying;
    quint8 _byte;
    int _position;
    
    static QHash<QByteArray, const QMetaObject*> _metaObjects;
};

#endif /* defined(__interface__Bitstream__) */
