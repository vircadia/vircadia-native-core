//
//  ByteCountCoding.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 6/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//
// TODO:
//   * handle signed types better, check for - numeric_limits<type>::is_signed
//   * test extra long buffer with garbage data at end...

#ifndef hifi_ByteCountCoding_h
#define hifi_ByteCountCoding_h

#include <algorithm>
#include <cassert>
#include <climits>
#include <limits>

#include <QDebug>

#include <QBitArray>
#include <QByteArray>

#include "SharedUtil.h"

#include "NumericalConstants.h"

template<typename T> class ByteCountCoded {
public:
    T data;
    ByteCountCoded(T input = 0) : data(input) { 
        // only use this template for non-signed integer types
        assert(!std::numeric_limits<T>::is_signed && std::numeric_limits<T>::is_integer);
    };

    ByteCountCoded(const QByteArray& fromEncoded) : data(0) { decode(fromEncoded); }

    QByteArray encode() const;
    size_t decode(const QByteArray& fromEncoded);
    size_t decode(const char* encodedBuffer, int encodedSize);

    bool operator==(const ByteCountCoded& other) const { return data == other.data; }
    bool operator!=(const ByteCountCoded& other) const { return data != other.data; }
    bool operator!() const { return data == 0; }

     operator QByteArray() const { return encode(); };
     operator T() const { return data; };
};

template<typename T> inline QByteArray& operator<<(QByteArray& out, const ByteCountCoded<T>& value) {
    return out = value;
}

template<typename T> inline QByteArray& operator>>(QByteArray& in, ByteCountCoded<T>& value) {
    value.decode(in);
    return in;
}

template<typename T> inline QByteArray ByteCountCoded<T>::encode() const {
    QByteArray output;

    int totalBits = sizeof(data) * BITS_IN_BYTE;
    int valueBits = totalBits;
    bool firstValueFound = false;
    T temp = data;
    T lastBitMask = (T)(1) << (totalBits - 1);

    // determine the number of bits that the value takes
    for (int bitAt = 0; bitAt < totalBits; bitAt++) {
        T bitValue = (temp & lastBitMask) == lastBitMask;
        if (!firstValueFound) {
            if (bitValue == 0) {
                valueBits--;
            } else {
                firstValueFound = true;
            }
        }
        temp = temp << 1;
    }
    
    // calculate the number of total bytes, including our header
    // BITS_IN_BYTE-1 because we need to code the number of bytes in the header
    // + 1 because we always take at least 1 byte, even if number of bits is less than a bytes worth
    int numberOfBytes = (valueBits / (BITS_IN_BYTE - 1)) + 1; 

    output.fill(0, numberOfBytes);

    // next pack the number of header bits in, the first N-1 to be set to 1, the last to be set to 0
    for(int i = 0; i < numberOfBytes; i++) {
        int outputIndex = i;
        T bitValue = (i < (numberOfBytes - 1)  ? 1 : 0);
        char original = output.at(outputIndex / BITS_IN_BYTE);
        int shiftBy = BITS_IN_BYTE - ((outputIndex % BITS_IN_BYTE) + 1);
        char thisBit = ( bitValue << shiftBy);
        output[i / BITS_IN_BYTE] = (original | thisBit);
    }

    // finally pack the the actual bits from the bit array
    temp = data;
    for(int i = numberOfBytes; i < (numberOfBytes + valueBits); i++) {
        int outputIndex = i;
        T bitValue = (temp & 1);
        char original = output.at(outputIndex / BITS_IN_BYTE);
        int shiftBy = BITS_IN_BYTE - ((outputIndex % BITS_IN_BYTE) + 1);
        char thisBit = ( bitValue << shiftBy);
        output[i / BITS_IN_BYTE] = (original | thisBit);

        temp = temp >> 1;
    }
    return output;
}

template<typename T> inline size_t ByteCountCoded<T>::decode(const QByteArray& fromEncodedBytes) {
    return decode(fromEncodedBytes.constData(), fromEncodedBytes.size());
}

template<typename T> inline size_t ByteCountCoded<T>::decode(const char* encodedBuffer, int encodedSize) {
    data = 0; // reset data
    size_t bytesConsumed = 0;
    int bitCount = BITS_IN_BYTE * encodedSize;

    int encodedByteCount = 1; // there is at least 1 byte (after the leadBits)
    int leadBits = 1; // there is always at least 1 lead bit
    bool inLeadBits = true;
    int bitAt = 0;
    int expectedBitCount; // unknown at this point
    int lastValueBit;
    T bitValue = 1;
    
    for(int byte = 0; byte < encodedSize; byte++) {
        char originalByte = encodedBuffer[byte];
        bytesConsumed++;
        unsigned char maskBit = 128; // LEFT MOST BIT set
        for(int bit = 0; bit < BITS_IN_BYTE; bit++) {
            bool bitIsSet = originalByte & maskBit;
            
            // Processing of the lead bits
            if (inLeadBits) {
                if (bitIsSet) {
                    encodedByteCount++;
                    leadBits++;
                } else {
                    inLeadBits = false; // once we hit our first 0, we know we're out of the lead bits
                    expectedBitCount = (encodedByteCount * BITS_IN_BYTE) - leadBits;
                    lastValueBit = expectedBitCount + bitAt;

                    // check to see if the remainder of our buffer is sufficient
                    if (expectedBitCount > (bitCount - leadBits)) {
                        break;
                    }
                }
            } else {
                if (bitAt > lastValueBit) {
                    break;
                }
                
                if(bitIsSet) {
                    data += bitValue;
                }
                bitValue *= 2;
            }
            bitAt++;
            maskBit = maskBit >> 1;
        }
        if (!inLeadBits && bitAt > lastValueBit) {
            break;
        }
    }
    return bytesConsumed;
}
#endif // hifi_ByteCountCoding_h

