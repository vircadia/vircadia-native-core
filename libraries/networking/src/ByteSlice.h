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

#ifndef networking_Byteslice_h
#define networking_Byteslice_h

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
    constexpr static size_t NPOS = static_cast<size_t>(-1);

    inline ByteSlice() : _offset(0), _length(0) {}
    inline ByteSlice(const QByteArray& data) : _offset(0), _length(data.length()), _content(ByteStringPointer::create(reinterpret_cast<const quint8*>(data.constData()), data.length())) {}
    inline ByteSlice(const std::string& data) : _offset(0), _length(data.length()), _content(ByteStringPointer::create(reinterpret_cast<const quint8*>(data.c_str()), data.length())) {}
    inline ByteSlice(const ByteSlice& data) : _offset(data._offset), _length(data._length), _content(data._content) {}
    inline ByteSlice(ByteSlice&& data) noexcept : _offset(data._offset), _length(data._length) { _content.swap(data._content); }
    void* create(size_t length);  // create a new buffer and return a pointer to it

    inline size_t length() const { return _length; }
    inline bool empty() const { return !_length; }
    void clear();
    inline const quint8* constData() const { return _content.isNull() ? NULL : _content->_content + _offset; }
    inline const quint8& operator[](size_t idx) const { return (_content.isNull() || idx > _length) ? _fallback : _content->_content[idx + _offset]; }

    quint8 pop_front();
    ByteSlice substring(size_t offset, size_t length = NPOS) const;
    ByteSlice concat(const ByteSlice& rhs) const;

private:
    class ByteString {
    public:
        const quint8* _content;
        size_t _fullLength;

        ByteString(const quint8* content, size_t length);
        ByteString(size_t length);
        ~ByteString();
    };
    typedef QSharedPointer<ByteString> ByteStringPointer;
    inline ByteSlice(ByteStringPointer content, size_t offset, size_t length)  : _offset(offset), _length(length), _content(content) {}

    static quint8 _fallback;

    ByteStringPointer _content;
    size_t _offset;
    size_t _length;
};

#endif /* networking_Byteslice_h */