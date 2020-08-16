//
//  Byteslice.cpp
//  libraries/networking/src
//
//  Created by Heather Anderson on 2020-05-01.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Byteslice.h"

quint8 ByteSlice::_fallback{ 0 };

ByteSlice::ByteString::ByteString(const quint8* source, size_t length) {
    quint8* myContent = new quint8[length];
    memcpy(myContent, source, length);
    _content = myContent;
    _fullLength = length;
}

ByteSlice::ByteString::ByteString(size_t length) {
    quint8* myContent = new quint8[length];
    memset(myContent, 0, length);
    _content = myContent;
    _fullLength = length;
}

ByteSlice::ByteString::~ByteString() {
    delete[] _content;
}

void ByteSlice::clear() {
    _offset = 0;
    _length = 0;
    _content.reset();
}

quint8 ByteSlice::pop_front() {
    if (!_length) {
        return 0;
    }
    --_length;
    return _content->_content[_offset++];
}

ByteSlice ByteSlice::substring(size_t offset, size_t length) const {
    if (offset >= _length) {
        return ByteSlice();
    }
    size_t maxLen = _length - offset;
    if (length == NPOS) {
        return ByteSlice(_content, _offset + offset, maxLen);
    } else {
        return ByteSlice(_content, _offset + offset, std::min(maxLen, length));
    }
}

// create a new buffer and return a pointer to it
void* ByteSlice::create(size_t length) {
    _offset = 0;
    _length = length;
    _content = ByteStringPointer::create(length);
    return const_cast<quint8*>(_content->_content);
}

// return a ByteSlice representing the concatenation of us and our argument.
// In most cases will construct a new Bytestring, but will attempt to avoid doing so
ByteSlice ByteSlice::concat(const ByteSlice& rhs) const {
    // first of all check if either side is empty, if so we don't need to do anything
    if (empty()) {
        return rhs;
    }
    if (rhs.empty()) {
        return *this;
    }

    // next check to see if we're "lucky" and we already have the data within ourselves
    size_t rhsLength = rhs.length();
    if (_offset + _length + rhsLength <= _content->_fullLength && memcmp(&_content->_content[_offset+_length], rhs.constData(), rhsLength) == 0) {
        // "the answer was within you the whole time"
        return ByteSlice(_content, _offset, _length + rhsLength);
    }
    if (rhs._offset >= _length && memcmp(&rhs._content->_content[rhs._offset - _length], constData(), _length) == 0) {
        // "the answer was within *them* the whole time"
        return ByteSlice(_content, rhs._offset - _length, _length + rhsLength);
    }

    // okay, didn't work.  Just create a new one from scratch
    ByteSlice newSlice;
    quint8* newContent = static_cast<quint8*>(newSlice.create(_length + rhsLength));
    memcpy(newContent, constData(), _length);
    memcpy(&newContent[_length], rhs.constData(), rhsLength);
    return newSlice;
}
