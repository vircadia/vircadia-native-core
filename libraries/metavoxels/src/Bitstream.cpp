//
//  Bitstream.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QDataStream>

#include "Bitstream.h"

Bitstream::Bitstream(QDataStream& underlying)
    : _underlying(underlying), _byte(0), _position(0) {
}

const int BITS_IN_BYTE = 8;
const int LAST_BIT_POSITION = BITS_IN_BYTE - 1;

Bitstream& Bitstream::write(const void* data, int bits, int offset) {
    const quint8* source = (const quint8*)data;
    while (bits > 0) {
        int bitsToWrite = qMin(BITS_IN_BYTE - _position, qMin(BITS_IN_BYTE - offset, bits));
        _byte |= ((*source >> offset) & ((1 << bitsToWrite) - 1)) << _position;
        if ((_position += bitsToWrite) == BITS_IN_BYTE) {
            flush();
        }
        if ((offset += bitsToWrite) == BITS_IN_BYTE) {
            source++;
            offset = 0;
        }
        bits -= bitsToWrite;
    }
    return *this;
}

Bitstream& Bitstream::read(void* data, int bits, int offset) {
    quint8* dest = (quint8*)data;
    while (bits > 0) {
        if (_position == 0) {
            _underlying >> _byte;
        }
        int bitsToRead = qMin(BITS_IN_BYTE - _position, qMin(BITS_IN_BYTE - offset, bits));
        *dest |= ((_byte >> _position) & ((1 << bitsToRead) - 1)) << offset;
        _position = (_position + bitsToRead) & LAST_BIT_POSITION;
        if ((offset += bitsToRead) == BITS_IN_BYTE) {
            dest++;
            offset = 0;
        }
        bits -= bitsToRead;
    }
    return *this;
}

void Bitstream::flush() {
    if (_position != 0) {
        _underlying << _byte;
        _byte = 0;
        _position = 0;
    }
}


Bitstream& Bitstream::operator<<(bool value) {
    if (value) {
        _byte |= (1 << _position);
    }
    if (++_position == BITS_IN_BYTE) {
        flush();
    }
    return *this;
}

Bitstream& Bitstream::operator>>(bool& value) {
    if (_position == 0) {
        _underlying >> _byte;
    }
    value = _byte & (1 << _position);
    _position = (_position + 1) & LAST_BIT_POSITION;
    return *this;
}
