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
//   * implement decode
//   * add operators

#ifndef hifi_PropertyFlags_h
#define hifi_PropertyFlags_h

#include <QBitArray>
#include <QByteArray>

template<typename Enum>class PropertyFlags {
public:
    typedef Enum enum_type;
    inline PropertyFlags() : _maxFlag(-1) { };
    inline PropertyFlags(const PropertyFlags& other) : _flags(other._flags), _maxFlag(other._maxFlag) {}
    inline PropertyFlags(Enum flag) : _maxFlag(-1) { setHasProperty(flag); }

    void clear() { _flags.clear(); _maxFlag = -1; }

    void setHasProperty(Enum flag, bool value = true);
    bool getHasProperty(Enum flag);
    QByteArray encode();
    void decode(const QByteArray& fromEncoded);


    inline PropertyFlags& operator=(const PropertyFlags &other) { _flags = other._flags; _maxFlag = other._maxFlag; return *this; }

    inline PropertyFlags& operator|=(PropertyFlags other) { _flags |= other._flags; _maxFlag = max(_maxFlag, other._maxFlag); return *this; }
    inline PropertyFlags& operator|=(Enum flag) { PropertyFlags other(flag); _flags |= other._flags; _maxFlag = max(_maxFlag, other._maxFlag); return *this; }

    inline PropertyFlags& operator&=(PropertyFlags other) { _flags &= other._flags; shinkIfNeeded(); return *this; }
    inline PropertyFlags& operator&=(Enum flag) { PropertyFlags other(flag); _flags &= other._flags; shinkIfNeeded(); return *this; }

    inline PropertyFlags& operator^=(PropertyFlags other) { _flags ^= other._flags; shinkIfNeeded(); return *this; }
    inline PropertyFlags& operator^=(Enum flag) { PropertyFlags other(flag); _flags ^= other._flags; shinkIfNeeded(); return *this; }


    inline PropertyFlags operator|(PropertyFlags other) const { PropertyFlags result(*this); result |= other; return result; }
    inline PropertyFlags operator|(Enum flag) const { PropertyFlags result(*this); PropertyFlags other(flag); result |= other; return result; }

    inline PropertyFlags operator^(PropertyFlags other) const { PropertyFlags result(*this); result ^= other; return result; }
    inline PropertyFlags operator^(Enum flag) const { PropertyFlags result(*this); PropertyFlags other(flag); result ^= other; return result; }

    inline PropertyFlags operator&(PropertyFlags other) const { PropertyFlags result(*this); result &= other; return result; }
    inline PropertyFlags operator&(Enum flag) const { PropertyFlags result(*this); PropertyFlags other(flag); result &= other; return result; }

    /*
    inline PropertyFlags operator~() const { return PropertyFlags(Enum(~i)); }
    inline bool operator!() const { return !i; }
    */


private:
    void shinkIfNeeded();

    QBitArray _flags;
    int _maxFlag;
};


template<typename Enum> inline void PropertyFlags<Enum>::setHasProperty(Enum flag, bool value) {
    if (flag > _maxFlag) {
        if (value) {
            _maxFlag = flag;
            _flags.resize(_maxFlag + 1);
        } else {
            return; // bail early, we're setting a flag outside of our current _maxFlag to false, which is already the default
        }
    }
    qDebug() << "_flags.setBit("<<flag<<")=" << value;
    _flags.setBit(flag, value);
    
    if (flag == _maxFlag && !value) {
        shinkIfNeeded();
    }
}

template<typename Enum> inline bool PropertyFlags<Enum>::getHasProperty(Enum flag) {
    if (flag > _maxFlag) {
        return false;
    }
    return _flags.testBit(flag);
}

const int BITS_PER_BYTE = 8;

template<typename Enum> inline QByteArray PropertyFlags<Enum>::encode() {
    const bool debug = false;
    QByteArray output;

    outputBufferBits((const unsigned char*)output.constData(), output.size());
    
    if (debug) qDebug() << "PropertyFlags<Enum>::encode()";
    
    // we should size the array to the correct size.
    int lengthInBytes = (_maxFlag / (BITS_PER_BYTE - 1)) + 1;

    if (debug) qDebug() << "    lengthInBytes=" << lengthInBytes;

    output.fill(0, lengthInBytes);

    if (debug) outputBufferBits((const unsigned char*)output.constData(), output.size());
    
    // next pack the number of header bits in, the first N-1 to be set to 1, the last to be set to 0
    for(int i = 0; i < lengthInBytes; i++) {
        int outputIndex = i;
        if (debug) qDebug() << "outputIndex:" << outputIndex;
        int bitValue = (i < (lengthInBytes - 1)  ? 1 : 0);

        if (debug) qDebug() << "    length code bit["<< outputIndex << "]=" << bitValue;

        char original = output.at(outputIndex / BITS_PER_BYTE);
        int shiftBy = BITS_PER_BYTE - ((outputIndex % BITS_PER_BYTE) + 1);
        char thisBit = ( bitValue << shiftBy);
        
        if (debug) {
            qDebug() << "bitValue:" << bitValue;
            qDebug() << "shiftBy:" << shiftBy;
            qDebug() << "original:";
            outputBits(original);

            qDebug() << "thisBit:";
            outputBits(thisBit);
        }

        output[i / BITS_PER_BYTE] = (original | thisBit);
    }

    if (debug) outputBufferBits((const unsigned char*)output.constData(), output.size());

    // finally pack the the actual bits from the bit array
    for(int i = lengthInBytes; i < (lengthInBytes + _maxFlag + 1); i++) {
        int flagIndex = i - lengthInBytes;
        int outputIndex = i;
        if (debug) qDebug() << "flagIndex:" << flagIndex;
        if (debug) qDebug() << "outputIndex:" << outputIndex;


        int bitValue = ( _flags[flagIndex]  ? 1 : 0);

        if (debug) qDebug() << "    encode bit["<<outputIndex<<"] = flags bit["<< flagIndex << "]=" << bitValue;
        
        char original = output.at(outputIndex / BITS_PER_BYTE);
        int shiftBy = BITS_PER_BYTE - ((outputIndex % BITS_PER_BYTE) + 1);
        char thisBit = ( bitValue << shiftBy);
        
        if (debug) {
            qDebug() << "bitValue:" << bitValue;
            qDebug() << "shiftBy:" << shiftBy;
            qDebug() << "original:";
            outputBits(original);

            qDebug() << "thisBit:";
            outputBits(thisBit);
        }
        
        output[i / BITS_PER_BYTE] = (original | thisBit);
    }

    if (debug) outputBufferBits((const unsigned char*)output.constData(), output.size());
    
    return output;
}

template<typename Enum> inline void PropertyFlags<Enum>::decode(const QByteArray& fromEncoded) {
}

template<typename Enum> inline void PropertyFlags<Enum>::shinkIfNeeded() {
    bool maxFlagWas = _maxFlag;
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



/***

BitArr.resize(8*byteArr.count());
for(int i=0; i<byteArr.count(); ++i)
    for(int b=0; b<8; ++b)
        bitArr.setBit(i*8+b, byteArr.at(i)&(1<<b));
Convertion from bits to bytes:

for(int i=0; i<bitArr.count(); ++i)
    byteArr[i/8] = (byteArr.at(i/8) | ((bitArr[i]?1:0)<<(i%8)));


template<typename Enum>
class QFlags
{
    typedef void **Zero;
    int i;
public:
    typedef Enum enum_type;
    Q_DECL_CONSTEXPR inline QFlags(const QFlags &f) : i(f.i) {}
    Q_DECL_CONSTEXPR inline QFlags(Enum f) : i(f) {}
    Q_DECL_CONSTEXPR inline QFlags(Zero = 0) : i(0) {}
    inline QFlags(QFlag f) : i(f) {}

    inline QFlags &operator=(const QFlags &f) { i = f.i; return *this; }
    inline QFlags &operator&=(int mask) { i &= mask; return *this; }
    inline QFlags &operator&=(uint mask) { i &= mask; return *this; }
    inline QFlags &operator|=(QFlags f) { i |= f.i; return *this; }
    inline QFlags &operator|=(Enum f) { i |= f; return *this; }
    inline QFlags &operator^=(QFlags f) { i ^= f.i; return *this; }
    inline QFlags &operator^=(Enum f) { i ^= f; return *this; }

    Q_DECL_CONSTEXPR  inline operator int() const { return i; }

    Q_DECL_CONSTEXPR inline QFlags operator|(QFlags f) const { return QFlags(Enum(i | f.i)); }
    Q_DECL_CONSTEXPR inline QFlags operator|(Enum f) const { return QFlags(Enum(i | f)); }
    Q_DECL_CONSTEXPR inline QFlags operator^(QFlags f) const { return QFlags(Enum(i ^ f.i)); }
    Q_DECL_CONSTEXPR inline QFlags operator^(Enum f) const { return QFlags(Enum(i ^ f)); }
    Q_DECL_CONSTEXPR inline QFlags operator&(int mask) const { return QFlags(Enum(i & mask)); }
    Q_DECL_CONSTEXPR inline QFlags operator&(uint mask) const { return QFlags(Enum(i & mask)); }
    Q_DECL_CONSTEXPR inline QFlags operator&(Enum f) const { return QFlags(Enum(i & f)); }
    Q_DECL_CONSTEXPR inline QFlags operator~() const { return QFlags(Enum(~i)); }

    Q_DECL_CONSTEXPR inline bool operator!() const { return !i; }

    inline bool testFlag(Enum f) const { return (i & f) == f && (f != 0 || i == int(f) ); }
};

***/

#endif // hifi_PropertyFlags_h

