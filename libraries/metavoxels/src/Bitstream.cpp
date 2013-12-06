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

Bitstream& Bitstream::write(const void* data, int bits) {
    return *this;
}

Bitstream& Bitstream::read(void* data, int bits) {
    return *this;
}

void Bitstream::flush() {
    if (_position != 0) {
        _underlying << _byte;
        _byte = 0;
        _position = 0;
    }
}

const int LAST_BIT_POSITION = 7;

Bitstream& Bitstream::operator<<(bool value) {
    if (value) {
        _byte |= (1 << _position);
    }
    if (_position++ == LAST_BIT_POSITION) {
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
