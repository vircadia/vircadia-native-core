//
//  Byteslice.h
//  libraries/networking/src
//
//  Created by Heather Anderson on 2020-05-01.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef serialization_Byteslice_h
#define serialization_Byteslice_h

#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>
/* ByteSlice

   This class represents an immutable string of bytes similar to what one could use std::string or QByteArray for.  The distinction
   is the data is stored in a shared memory area and reallocations are avoided.  Copies do not involve dynamic memory allocations,
   operations such as "substring" also refer to the original without reallocation.  The intent is to ease passing around byte strings
   or pieces of byte strings that may be "sliced and diced" without worrying about memory thrashing.
*/
class ByteSlice {
public:
    inline ByteSlice();
    inline ByteSlice(const QByteArray& data);
    inline ByteSlice(const std::string& data);
    inline ByteSlice(const ByteSlice& data);
    void* create(size_t length);  // create a new buffer and return a pointer to it

    inline size_t length() const;
    inline bool empty() const;
    inline void clear();
    inline const quint8* constData() const;
    inline const quint8& operator[](size_t idx) const;

    quint8 pop_front();
    ByteSlice substring(size_t offset, size_t length = static_cast<size_t>(-1)) const;
    ByteSlice concat(const ByteSlice& rhs) const;

private:
    class Bytestring {
    public:
        const quint8* _content;
        size_t _fullLength;

        inline Bytestring(const quint8* content, size_t length);
        ~Bytestring();
    };
    typedef QSharedPointer<Bytestring> BytestringPointer;
    inline ByteSlice(BytestringPointer content, size_t offset, size_t length);

    static quint8 gl_fallback;

    BytestringPointer _content;
    size_t _offset;
    size_t _length;
};

#include "Byteslice.inl"
#endif /* serialization_Byteslice_h */