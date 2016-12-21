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

#include "ByteCountCoding.h"
#include "SharedLogging.h"

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
    bool isEmpty() const { return _maxFlag == INT_MIN && _minFlag == INT_MAX && _trailingFlipped == false && _encodedLength == 0; }

    Enum firstFlag() const { return (Enum)_minFlag; }
    Enum lastFlag() const { return (Enum)_maxFlag; }
    
    void setHasProperty(Enum flag, bool value = true);
    bool getHasProperty(Enum flag) const;
    QByteArray encode();
    size_t decode(const uint8_t* data, size_t length);
    size_t decode(const QByteArray& fromEncoded);

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
    // enum is not known, these operators will only perform their bitwise operations on the set of properties that have
    // been previously set
    PropertyFlags& operator^=(const PropertyFlags& other);
    PropertyFlags& operator^=(Enum flag);
    PropertyFlags operator^(const PropertyFlags& other) const;
    PropertyFlags operator^(Enum flag) const;
    PropertyFlags operator~() const;

    void debugDumpBits();

    int getEncodedLength() const { return _encodedLength; }


private:
    void shrinkIfNeeded();

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
        shrinkIfNeeded();
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

template<typename Enum> 
inline size_t PropertyFlags<Enum>::decode(const uint8_t* data, size_t size) {
    clear(); // we are cleared out!

    size_t bytesConsumed = 0;
    int bitCount = BITS_IN_BYTE * (int)size;

    int encodedByteCount = 1; // there is at least 1 byte (after the leadBits)
    int leadBits = 1; // there is always at least 1 lead bit
    bool inLeadBits = true;
    int bitAt = 0;
    int expectedBitCount; // unknown at this point
    int lastValueBit = 0;
    for (unsigned int byte = 0; byte < size; byte++) {
        char originalByte = data[byte];
        bytesConsumed++;
        unsigned char maskBit = 0x80; // LEFT MOST BIT set
        for (int bit = 0; bit < BITS_IN_BYTE; bit++) {
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

                if (bitIsSet) {
                    setHasProperty(static_cast<Enum>(bitAt - leadBits), true);
                }
            }
            bitAt++;
            maskBit >>= 1;
        }
        if (!inLeadBits && bitAt > lastValueBit) {
            break;
        }
    }
    _encodedLength = (int)bytesConsumed;
    return bytesConsumed;
}

template<typename Enum> inline size_t PropertyFlags<Enum>::decode(const QByteArray& fromEncodedBytes) {
    return decode(reinterpret_cast<const uint8_t*>(fromEncodedBytes.data()), fromEncodedBytes.size());
}

template<typename Enum> inline void PropertyFlags<Enum>::debugDumpBits() {
    qCDebug(shared) << "_minFlag=" << _minFlag;
    qCDebug(shared) << "_maxFlag=" << _maxFlag;
    qCDebug(shared) << "_trailingFlipped=" << _trailingFlipped;
    QString bits;
    for(int i = 0; i < _flags.size(); i++) {
        bits += (_flags.at(i) ? "1" : "0");
    }
    qCDebug(shared) << "bits:" << bits;
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
    shrinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator&=(Enum flag) {
    PropertyFlags other(flag); 
    _flags &= other._flags; 
    shrinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator^=(const PropertyFlags& other) {
    _flags ^= other._flags; 
    shrinkIfNeeded(); 
    return *this; 
}

template<typename Enum> inline PropertyFlags<Enum>& PropertyFlags<Enum>::operator^=(Enum flag) {
    PropertyFlags other(flag); 
    _flags ^= other._flags; 
    shrinkIfNeeded(); 
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

template<typename Enum> inline void PropertyFlags<Enum>::shrinkIfNeeded() {
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

