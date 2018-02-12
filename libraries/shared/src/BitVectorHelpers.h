//
//  BitVectorHelpers.h
//  libraries/shared/src
//
//  Created by Anthony Thibault on 1/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BitVectorHelpers_h
#define hifi_BitVectorHelpers_h

#include "NumericalConstants.h"

int calcBitVectorSize(int numBits) {
    return ((numBits - 1) >> 3) + 1;
}

// func should be of type bool func(int index)
template <typename F>
int writeBitVector(uint8_t* destinationBuffer, int numBits, const F& func) {
    int totalBytes = calcBitVectorSize(numBits);
    uint8_t* cursor = destinationBuffer;
    uint8_t byte = 0;
    uint8_t bit = 0;

    for (int i = 0; i < numBits; i++) {
        if (func(i)) {
            byte |= (1 << bit);
        }
        if (++bit == BITS_IN_BYTE) {
            *cursor++ = byte;
            byte = 0;
            bit = 0;
        }
    }
    // write the last byte, if necessary
    if (bit != 0) {
        *cursor++ = byte;
    }

    assert((int)(cursor - destinationBuffer) == totalBytes);
    return totalBytes;
}

// func should be of type 'void func(int index, bool value)'
template <typename F>
int readBitVector(const uint8_t* sourceBuffer, int numBits, const F& func) {
    int totalBytes = calcBitVectorSize(numBits);
    const uint8_t* cursor = sourceBuffer;
    uint8_t bit = 0;

    for (int i = 0; i < numBits; i++) {
        bool value = (bool)(*cursor & (1 << bit));
        func(i, value);
        if (++bit == BITS_IN_BYTE) {
            cursor++;
            bit = 0;
        }
    }
    return totalBytes;
}

#endif
