//
//  Created by Bradley Austin Davis on 2015/07/08
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_BufferParser_h
#define hifi_BufferParser_h

#include <cstdint>

#include <QUuid>
#include <QtEndian>

#include "GLMHelpers.h"
#include "ByteCountCoding.h"
#include "PropertyFlags.h"
#include <glm/gtc/type_ptr.hpp>

class BufferParser {
public:
    BufferParser(const uint8_t* data, size_t size, size_t offset = 0) :
        _offset(offset), _data(data), _size(size) {
    }

    template<typename T>
    inline void readValue(T& result) {
        Q_ASSERT(remaining() >= sizeof(T));
        memcpy(&result, _data + _offset, sizeof(T));
        _offset += sizeof(T);
    }

    inline void readUuid(QUuid& result) {
        readValue(result.data1);
        readValue(result.data2);
        readValue(result.data3);
        readValue(result.data4);
        result.data1 = qFromBigEndian<quint32>(result.data1);
        result.data2 = qFromBigEndian<quint16>(result.data2);
        result.data3 = qFromBigEndian<quint16>(result.data3);
    }

    template <typename T>
    inline void readFlags(PropertyFlags<T>& result) {
        _offset += result.decode(_data + _offset, remaining());
    }

    template<typename T>
    inline void readCompressedCount(T& result) {
        // FIXME switch to a heapless implementation as soon as Brad provides it.
        ByteCountCoded<T> codec;
        _offset += codec.decode(reinterpret_cast<const char*>(_data + _offset), (int)remaining());
        result = codec.data;
    }

    inline size_t remaining() const {
        return _size - _offset;
    }

    inline size_t offset() const {
        return _offset;
    }

    inline const uint8_t* data() const {
        return _data;
    }

private:

    size_t _offset{ 0 };
    const uint8_t* const _data;
    const size_t _size;
};


template<>
inline void BufferParser::readValue<quat>(quat& result) {
    size_t advance = unpackOrientationQuatFromBytes(_data + _offset, result);
    _offset += advance;
}

template<>
inline void BufferParser::readValue(QString& result) {
    uint16_t length; readValue(length);
    result = QString((const char*)_data + _offset);
}

template<>
inline void BufferParser::readValue(QUuid& result) {
    uint16_t length; readValue(length);
    Q_ASSERT(16 == length);
    readUuid(result);
}

template<>
inline void BufferParser::readValue(QVector<glm::vec3>& result) {
    uint16_t length; readValue(length);
    result.resize(length);
    for (int i=0; i<length; i++) {
        memcpy(glm::value_ptr(result[i]), _data + _offset + (sizeof(glm::vec3)*i), sizeof(glm::vec3) * length);
    }

    _offset += sizeof(glm::vec3) * length;
}

template<>
inline void BufferParser::readValue(QByteArray& result) {
    uint16_t length; readValue(length);
    result = QByteArray((char*)_data + _offset, (int)length);
    _offset += length;
}

#endif
