//
//  Byteslice.inl
//  libraries/networking/src
//
//  Created by Heather Anderson on 2020-05-01.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef serialization_Byteslice_inl
#define serialization_Byteslice_inl

#include "Byteslice.h"

inline ByteSlice::ByteSlice() : _offset(0), _length(0) {
}

inline ByteSlice::ByteSlice(const QByteArray& data) :
    _offset(0), _length(data.length()),
    _content(BytestringPointer::create(reinterpret_cast<const quint8*>(data.constData()), _length)) {
}

inline ByteSlice::ByteSlice(const std::string& data) :
    _offset(0), _length(data.length()),
    _content(BytestringPointer::create(reinterpret_cast<const quint8*>(data.c_str()), _length)) {
}

inline ByteSlice::ByteSlice(BytestringPointer content, size_t offset, size_t length) :
    _offset(offset), _length(length), _content(content) {
}

// internal-only, for creating substrings
inline ByteSlice::ByteSlice(const ByteSlice& data) : _offset(data._offset), _length(data._length), _content(data._content) {
}

inline void ByteSlice::clear() {
    _offset = 0;
    _length = 0;
    _content.reset();
}

inline bool ByteSlice::empty() const {
    return !_length;
}

inline size_t ByteSlice::length() const {
    return _length;
}

inline const quint8& ByteSlice::operator[](size_t idx) const {
    if (_content.isNull() || idx > _length) {
        return gl_fallback;
    } else {
        return _content->_content[idx];
    }
}

inline const quint8* ByteSlice::constData() const {
    if (_content.isNull()) {
        return NULL;
    } else {
        return _content->_content;
    }
}

#endif /* serialization_Byteslice_inl */