//
//  SeqNum.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SeqNum_h
#define hifi_SeqNum_h

#include <glm/detail/func_common.hpp>

namespace udt {

class SeqNum {
public:
    // Base type of sequence numbers
    using Type = uint32_t;
    
    // Values are for 29 bit SeqNum
    static const Type THRESHOLD = 0x0FFFFFFF; // threshold for comparing sequence numbers
    static const Type MAX = 0x1FFFFFFF; // maximum sequence number used in UDT
    
    SeqNum() : _value(0) {}
    
    // Only explicit conversions
    explicit SeqNum(Type value) { *this = value; }
    explicit operator Type() { return _value; }
    
    inline SeqNum& operator++() {
        _value = (_value == MAX) ? 0 : ++_value;
        return *this;
    }
    inline SeqNum& operator--() {
        _value = (_value == 0) ? MAX : --_value;
        return *this;
    }
    inline SeqNum operator++(int) {
        SeqNum before = *this;
        (*this)++;
        return before;
    }
    inline SeqNum operator--(int) {
        SeqNum before = *this;
        (*this)--;
        return before;
    }
    
    inline SeqNum& operator=(Type value) {
        _value = (value <= MAX) ? value : MAX;
        return *this;
    }
    inline SeqNum& operator=(SeqNum& other) {
        _value = other._value;
        return *this;
    }
    inline SeqNum& operator+=(Type inc) {
        _value = (_value + inc > MAX) ? _value + inc - (MAX + 1) : _value + inc;
        return *this;
    }
    inline SeqNum& operator-=(Type dec) {
        _value = (_value < dec) ? MAX - (dec - _value + 1) : _value - dec;
        return *this;
    }
    
    friend int seqcmp(const SeqNum& seq1, const SeqNum& seq2);
    friend int seqlen(const SeqNum& seq1, const SeqNum& seq2);
    friend int seqoff(const SeqNum& seq1, const SeqNum& seq2);
    
private:
    Type _value;
};
    
int seqcmp(const SeqNum& seq1, const SeqNum& seq2);
int seqlen(const SeqNum& seq1, const SeqNum& seq2);
int seqoff(const SeqNum& seq1, const SeqNum& seq2);

}

#endif // hifi_SeqNum_h
