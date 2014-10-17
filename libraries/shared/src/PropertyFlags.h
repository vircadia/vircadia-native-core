//
//  PropertyFlags.h
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
//   * consider adding iterator to enumerate the properties that have been set?
//   * operator QSet<Enum> - this would be easiest way to handle enumeration
//   * make encode(), QByteArray<< operator, and QByteArray operator const by moving calculation of encoded length to
//     setFlag() and other calls
//   * should the QByteArray<< operator and QByteArray>> operator return the shifted versions of the byte arrays?

#ifndef hifi_PropertyFlags_h
#define hifi_PropertyFlags_h

#include <algorithm>
#include <climits>

#include <QBitArray>
#include <QByteArray>

#include <SharedUtil.h>

template<typename Enum>class PropertyFlags {
public:
    typedef Enum enum_type;
    inline PropertyFlags() : 
            _maxFlag(INT_MIN), _minFlag(INT_MAX), _trailingFlipped(false), _encodedLength(0) { };

    inline PropertyFlags(const PropertyFlags& other) : 
            _flags(other._flags), _maxFlag(other._maxFlag), _minFlag(other._minFlag), 
            _trailingFlipped(other._trailingFlipped), _encodedLength(0) {}

    inline PropertyFlags(Enum flag) : 
            _maxFlag(INT_MIN), _minFlag(INT_MAX), _trailingFlipped(false), _encodedLength(0) { setHasProperty(flag); }

    inline PropertyFlags(const QByteArray& fromEncoded) : 
            _maxFlag(INT_MIN), _minFlag(INT_MAX), _trailingFlipped(false), _encodedLength(0) { decode(fromEncoded); }

    void clear() { _flags.clear(); _maxFlag = INT_MIN; _minFlag = INT_MAX; _trailingFlipped = false; _encodedLength = 0; }

    Enum firstFlag() const { return (Enum)_minFlag; }
    Enum lastFlag() const { return (Enum)_maxFlag; }
    
    void setHasProperty(Enum flag, bool value = true);
    bool getHasProperty(Enum flag) const;
    QByteArray encode();
    void decode(const QByteArray& fromEncoded);

    operator QByteArray() { return encode(); };

    bool operator==(const PropertyFlags& other) const { return _flags == other._flags; }
    bool operator!=(const PropertyFlags& other) const { return _flags != other._flags; }
    bool operator!() const { return _flags.size() == 0; }

    PropertyFlags& operator=(const PropertyFlags& other);

    PropertyFlags& operator|=(const PropertyFlags& other);
    PropertyFlags& operator|=(Enum flag);

    PropertyFlags& operator&=(const PropertyFlags& other);
    PropertyFlags& operator&=(Enum flag);

    PropertyFlags& operator+=(const PropertyFlags& other);
    PropertyFlags& operator+=(Enum flag);

    PropertyFlags& operator-=(const PropertyFlags& other);
    PropertyFlags& operator-=(Enum flag);

    PropertyFlags& operator<<=(const PropertyFlags& other);
    PropertyFlags& operator<<=(Enum flag);

    PropertyFlags operator|(const PropertyFlags& other) const;
    PropertyFlags operator|(Enum flag) const;

    PropertyFlags operator&(const PropertyFlags& other) const;
    PropertyFlags operator&(Enum flag) const;

    PropertyFlags operator+(const PropertyFlags& other) const;
    PropertyFlags operator+(Enum flag) const;

    PropertyFlags operator-(const PropertyFlags& other) const;
    PropertyFlags operator-(Enum flag) const;

    PropertyFlags operator<<(const PropertyFlags& other) const;
    PropertyFlags operator<<(Enum flag) const;

    // NOTE: due to the nature of the compact storage of these property flags, and the fact that the upper bound of the
    // enum is not know, these operators will only perform their bitwise operations on the set of properties that have
    // been previously set
    PropertyFlags& operator^=(const PropertyFlags& other);
    PropertyFlags& operator^=(Enum flag);
    PropertyFlags operator^(const PropertyFlags& other) const;
    PropertyFlags operator^(Enum flag) const;
    PropertyFlags operator~() const;

    void debugDumpBits();

    int getEncodedLength() const { return _encodedLength; }


private:
    void shinkIfNeeded();

    QBitArray _flags;
    int _maxFlag;
    int _minFlag;
    bool _trailingFlipped; /// are the trailing properties flipping in their state (e.g. assumed true, instead of false)
    int _encodedLength;
};

template<typename Enum> PropertyFlags<Enum>& operator<<(PropertyFlags<Enum>& out, const PropertyFlags<Enum>& other) {
    return out <<= other;
}

template<typename Enum> PropertyFlags<Enum>& operator<<(PropertyFlags<Enum>& out, Enum flag) {
    return out <<= flag;
}


template<typename Enum> inline void PropertyFlags<Enum>::setHasProperty(Enum flag, bool value) {
    // keep track of our min flag
    if (flag < _minFlag) {
        if (value) {
            _minFlag = flag;
        }
    }
    if (flag > _maxFlag) {
        if (value) {
            _maxFlag = flag;
            _flags.resize(_maxFlag + 1);
        } else {
            return; // bail early, we're setting a flag outside of our current _maxFlag to false, which is already the default
        }
    }
    _flags.setBit(flag, value);
    
    if (flag == _maxFlag && !value) {
        shinkIfNeeded();
    }
}

template<typename Enum> inline bool PropertyFlags<Enum>::getHasProperty(Enum flag) const {
    if (flag > _maxFlag) {
        return _trailingFlipped; // usually false
    }
    return _flags.testBit(flag);
}

const int BITS_PER_BYTE = 8;

template<typename Enum> inline QByteArray PropertyFlags<Enum>::encode() {
    QByteArray output;
    
    if (_maxFlag < _minFlag) {
        output.fill(0, 1);
        return output; // no flags... nothing to encode
    }

    // we should size the array to the correct size.
    int lengthInBytes = (_maxFlag / (BITS_PER_BYTE - 1)) + 1;

    output.fill(0, lengthInBytes);

    // next pack the number of header bits in, the first N-1 to be set to 1, the last to be set to 0
    for(int i = 0; i < lengthInBytes; i++) {
        int outputIndex = i;
        int bitValue = (i < (lengthInBytes - 1)  ? 1 : 0);
        char original = output.at(outputIndex / BITS_PER_BYTE);
        int shiftBy = BITS_PER_BYTE - ((outputIndex % BITS_PER_BYTE) + 1);
        char thisBit = ( bitValue << shiftBy);
        output[i / BITS_PER_BYTE] = (original | thisBit);
    }

    // finally pack the the actual bits from the bit array
    for(int i = lengthInBytes; i < (lengthInBytes + _maxFlag + 1); i++) {
        int flagIndex = i - lengthInBytes;
        int outputIndex = i;
        int bitValue = ( _flags[flagIndex]  ? 1 : 0);
        char original = output.at(outputIndex / BITS_PER_BYTE);
        int shiftBy = BITS_PER_BYTE - ((outputIndex % BITS_PER_BYTE) + 1);
        char thisBit = ( bitValue << shiftBy);
        output[i / BITS_PER_BYTE] = (original | thisBit);
    }
    
    _encodedLength = lengthInBytes;
    return output;
}

template<typename Enum> inline void PropertyFlags<Enum>::decode(const QByteArray& fromEncodedBytes) {

    clear(); // we are cleared out!

    // first convert the ByteArray into a BitArray...
    QBitArray encodedBits;
    int bitCount = BITS_PER_BYTE * fromEncodedBytes.count();
    encodedBits.resize(bitCount);
    
    for(int byte = 0; byte < fromEncodedBytes.count(); byte++) {
        char originalByte = fromEncodedBytes.at(byte);
        for(int bit = 0; bit < BITS_PER_BYTE; bit++) {
            int shiftBy = BITS_PER_BYTE - (bit + 1);
            char maskBit = ( 1 << shiftBy);
            bool bitValue = originalByte & maskBit;
            encodedBits.setBit(byte * BITS_PER_BYTE + bit, bitValue);
        }
    }
    
    // next, read the leading bits to determine the correct number of bytes to decode (may not match the QByteArray)
    int encodedByteCount = 0;
    int leadBits = 1;
    int bitAt;
    for (bitAt = 0; bitAt < bitCount; bitAt++) {
        if (encodedBits.at(bitAt)) {
            encodedByteCount++;
            leadBits++;
        } else {
            break;
        }
    }
    encodedByteCount++; // always at least one byte
    _encodedLength = encodedByteCount;

    int expectedBitCount = encodedByteCount * BITS_PER_BYTE;
    
    // Now, keep reading...
    if (expectedBitCount <= (encodedBits.size() - leadBits)) {
        int flagsStartAt = bitAt + 1; 
        for (bitAt = flagsStartAt; bitAt < expectedBitCount; bitAt++) {
            if (encodedBits.at(bitAt)) {
                setHasProperty((Enum)(bitAt - flagsStartAt));
            }
        }
    }
}

template<typename Enum> inline void PropertyFlags<Enum>::debugDumpBits() {
    qDebug() << "_minFlag=" << _minFlag;
    qDebug() << "_maxFlag=" << _maxFlag;
    qDebug() << "_trailingFlipped=" << _trailingFlipped;
    for(int i = 0; i < _flags.size(); i++) {
        qDebug() << "bit[" << i << "]=" << _flags.at(i);
    }
}


template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator=(const PropertyFlags& other) {
    _flags = other._flags; 
    _maxFlag = other._maxFlag; 
    _minFlag = other._minFlag; 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator|=(const PropertyFlags& other) {
    _flags |= other._flags; 
    _maxFlag = std::max(_maxFlag, other._maxFlag); 
    _minFlag = std::min(_minFlag, other._minFlag); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator|=(Enum flag) {
    PropertyFlags other(flag); 
    _flags |= other._flags; 
    _maxFlag = std::max(_maxFlag, other._maxFlag); 
    _minFlag = std::min(_minFlag, other._minFlag); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator&=(const PropertyFlags& other) {
    _flags &= other._flags; 
    shinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator&=(Enum flag) {
    PropertyFlags other(flag); 
    _flags &= other._flags; 
    shinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator^=(const PropertyFlags& other) {
    _flags ^= other._flags; 
    shinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator^=(Enum flag) {
    PropertyFlags other(flag); 
    _flags ^= other._flags; 
    shinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator+=(const PropertyFlags& other) {
    for(int flag = (int)other.firstFlag(); flag <= (int)other.lastFlag(); flag++) {
        if (other.getHasProperty((Enum)flag)) {
            setHasProperty((Enum)flag, true);
        }
    }
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator+=(Enum flag) {
    setHasProperty(flag, true);
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator-=(const PropertyFlags& other) {
    for(int flag = (int)other.firstFlag(); flag <= (int)other.lastFlag(); flag++) {
        if (other.getHasProperty((Enum)flag)) {
            setHasProperty((Enum)flag, false);
        }
    }
    return *this;
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator-=(Enum flag) {
    setHasProperty(flag, false);
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator<<=(const PropertyFlags& other) {
    for(int flag = (int)other.firstFlag(); flag <= (int)other.lastFlag(); flag++) {
        if (other.getHasProperty((Enum)flag)) {
            setHasProperty((Enum)flag, true);
        }
    }
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator<<=(Enum flag) {
    setHasProperty(flag, true);
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator|(const PropertyFlags& other) const {
    PropertyFlags result(*this); 
    result |= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator|(Enum flag) const {
    PropertyFlags result(*this); 
    PropertyFlags other(flag); 
    result |= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator&(const PropertyFlags& other) const {
    PropertyFlags result(*this); 
    result &= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator&(Enum flag) const { 
    PropertyFlags result(*this); 
    PropertyFlags other(flag); 
    result &= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator^(const PropertyFlags& other) const {
    PropertyFlags result(*this); 
    result ^= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator^(Enum flag) const {
    PropertyFlags result(*this); 
    PropertyFlags other(flag); 
    result ^= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator+(const PropertyFlags& other) const {
    PropertyFlags result(*this); 
    result += other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator+(Enum flag) const { 
    PropertyFlags result(*this); 
    result.setHasProperty(flag, true);
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator-(const PropertyFlags& other) const {
    PropertyFlags result(*this); 
    result -= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator-(Enum flag) const { 
    PropertyFlags result(*this); 
    result.setHasProperty(flag, false);
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator<<(const PropertyFlags& other) const {
    PropertyFlags result(*this); 
    result <<= other; 
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator<<(Enum flag) const { 
    PropertyFlags result(*this); 
    result.setHasProperty(flag, true);
    return result; 
}

template<typename Enum> inline PropertyFlags<Enum> PropertyFlags<Enum>::operator~() const { 
    PropertyFlags result(*this); 
    result._flags = ~_flags;
    result._trailingFlipped = !_trailingFlipped;
    return result; 
}

template<typename Enum> inline void PropertyFlags<Enum>::shinkIfNeeded() {
    int maxFlagWas = _maxFlag;
    while (_maxFlag >= 0) {
        if (_flags.testBit(_maxFlag)) {
            break;
        }
        _maxFlag--;
    }
    if (maxFlagWas != _maxFlag) {
        _flags.resize(_maxFlag + 1);
    }
}

template<typename Enum> inline QByteArray& operator<<(QByteArray& out, PropertyFlags<Enum>& value) {
    return out = value;
}

template<typename Enum> inline QByteArray& operator>>(QByteArray& in, PropertyFlags<Enum>& value) {
    value.decode(in);
    return in;
}

#endif // hifi_PropertyFlags_h

