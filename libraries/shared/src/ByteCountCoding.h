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

#include <QBitArray>
#include <QByteArray>

#include "SharedUtil.h"

template<typename T> class ByteCountCoded {
public:
    T data;
    ByteCountCoded(T input = 0) : data(input) { 
        // only use this template for non-signed integer types
        assert(!std::numeric_limits<T>::is_signed && std::numeric_limits<T>::is_integer);
    };

    ByteCountCoded(const QByteArray& fromEncoded) : data(0) { decode(fromEncoded); }

    QByteArray encode() const;
    void decode(const QByteArray& fromEncoded);

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

    //qDebug() << "data=";
    //outputBufferBits((const unsigned char*)&data, sizeof(data));
    
    T totalBits = sizeof(data) * BITS_IN_BYTE;
    //qDebug() << "totalBits=" << totalBits;
    T valueBits = totalBits;
    bool firstValueFound = false;
    T temp = data;
    T lastBitMask = (T)(1) << (totalBits - 1);

    //qDebug() << "lastBitMask=";
    //outputBufferBits((const unsigned char*)&lastBitMask, sizeof(lastBitMask));

    // determine the number of bits that the value takes
    for (int bitAt = 0; bitAt < totalBits; bitAt++) {
        T bitValue = (temp & lastBitMask) == lastBitMask;
        //qDebug() << "bitValue[" << bitAt <<"]=" << bitValue;
        if (!firstValueFound) {
            if (bitValue == 0) {
                valueBits--;
            } else {
                firstValueFound = true;
            }
        }
        temp = temp << 1;
    }
    //qDebug() << "valueBits=" << valueBits;
    
    // calculate the number of total bytes, including our header
    // BITS_IN_BYTE-1 because we need to code the number of bytes in the header
    // + 1 because we always take at least 1 byte, even if number of bits is less than a bytes worth
    int numberOfBytes = (valueBits / (BITS_IN_BYTE - 1)) + 1; 
    //qDebug() << "numberOfBytes=" << numberOfBytes;

    //int numberOfBits = numberOfBytes + valueBits;
    //qDebug() << "numberOfBits=" << numberOfBits;
    
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

template<typename T> inline void ByteCountCoded<T>::decode(const QByteArray& fromEncodedBytes) {

    // first convert the ByteArray into a BitArray...
    QBitArray encodedBits;
    int bitCount = BITS_IN_BYTE * fromEncodedBytes.count();
    encodedBits.resize(bitCount);
    
    for(int byte = 0; byte < fromEncodedBytes.count(); byte++) {
        char originalByte = fromEncodedBytes.at(byte);
        for(int bit = 0; bit < BITS_IN_BYTE; bit++) {
            int shiftBy = BITS_IN_BYTE - (bit + 1);
            char maskBit = ( 1 << shiftBy);
            bool bitValue = originalByte & maskBit;
            encodedBits.setBit(byte * BITS_IN_BYTE + bit, bitValue);
        }
    }
    
    // next, read the leading bits to determine the correct number of bytes to decode (may not match the QByteArray)
    int encodedByteCount = 0;
    int bitAt;
    for (bitAt = 0; bitAt < bitCount; bitAt++) {
        if (encodedBits.at(bitAt)) {
            encodedByteCount++;
        } else {
            break;
        }
    }
    encodedByteCount++; // always at least one byte
    int expectedBitCount = encodedByteCount * BITS_IN_BYTE;
    
    // Now, keep reading...
    int valueStartsAt = bitAt + 1; 
    T value = 0;
    T bitValue = 1;
    for (bitAt = valueStartsAt; bitAt < expectedBitCount; bitAt++) {
        if(encodedBits.at(bitAt)) {
            value += bitValue;
        }
        bitValue *= 2;
    }
    data = value;
}
#endif // hifi_ByteCountCoding_h

