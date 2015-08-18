//
//  SequenceNumber.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SequenceNumber_h
#define hifi_SequenceNumber_h

#include <functional>

#include <glm/detail/func_common.hpp>

namespace udt {

class SequenceNumber {
public:
    // Base type of sequence numbers
    using Type = int32_t;
    using UType = uint32_t;
    
    // Values are for 29 bit SequenceNumber
    static const Type THRESHOLD = 0x0FFFFFFF; // threshold for comparing sequence numbers
    static const Type MAX = 0x1FFFFFFF; // maximum sequence number used in UDT
    
    SequenceNumber() = default;
    SequenceNumber(const SequenceNumber& other) : _value(other._value) {}
    
    // Only explicit conversions
    explicit SequenceNumber(char* value) { _value = (*reinterpret_cast<int32_t*>(value)) & MAX; }
    explicit SequenceNumber(Type value) { _value = (value <= MAX) ? ((value >= 0) ? value : 0) : MAX; }
    explicit SequenceNumber(UType value) { _value = (value <= MAX) ? value : MAX; }
    explicit operator Type() { return _value; }
    explicit operator UType() { return static_cast<UType>(_value); }
    
    inline SequenceNumber& operator++() {
        _value = (_value + 1) % (MAX + 1);
        return *this;
    }
    inline SequenceNumber& operator--() {
        _value = (_value == 0) ? MAX : --_value;
        return *this;
    }
    inline SequenceNumber operator++(int) {
        SequenceNumber before = *this;
        ++(*this);
        return before;
    }
    inline SequenceNumber operator--(int) {
        SequenceNumber before = *this;
        --(*this);
        return before;
    }
    
    inline SequenceNumber& operator=(const SequenceNumber& other) {
        _value = other._value;
        return *this;
    }
    inline SequenceNumber& operator+=(Type inc) {
        _value = (_value + inc > MAX) ? _value + inc - (MAX + 1) : _value + inc;
        return *this;
    }
    inline SequenceNumber& operator-=(Type dec) {
        _value = (_value < dec) ? MAX - (dec - _value + 1) : _value - dec;
        return *this;
    }
    
    inline bool operator==(const SequenceNumber& other) const {
        return _value == other._value;
    }
    inline bool operator!=(const SequenceNumber& other) const {
         return _value != other._value;
    }
    
    friend bool operator<(const SequenceNumber&  a, const SequenceNumber& b);
    friend bool operator>(const SequenceNumber&  a, const SequenceNumber& b);
    friend bool operator<=(const SequenceNumber&  a, const SequenceNumber& b);
    friend bool operator>=(const SequenceNumber&  a, const SequenceNumber& b);
    
    friend SequenceNumber operator+(const SequenceNumber a, const Type& b);
    friend SequenceNumber operator+(const Type& a, const SequenceNumber b);
    friend SequenceNumber operator-(const SequenceNumber a, const Type& b);
    friend SequenceNumber operator-(const Type& a, const SequenceNumber b);
    
    friend int seqlen(const SequenceNumber& seq1, const SequenceNumber& seq2);
    friend int seqoff(const SequenceNumber& seq1, const SequenceNumber& seq2);
    
private:
    Type _value { 0 };
    
    friend struct std::hash<SequenceNumber>;
};
static_assert(sizeof(SequenceNumber) == sizeof(uint32_t), "SequenceNumber invalid size");
    
    
inline bool operator<(const SequenceNumber&  a, const SequenceNumber& b) {
    return (glm::abs(a._value - b._value) < SequenceNumber::THRESHOLD) ? a._value < b._value : b._value < a._value;
}

inline bool operator>(const SequenceNumber&  a, const SequenceNumber& b) {
    return (glm::abs(a._value - b._value) < SequenceNumber::THRESHOLD) ? a._value > b._value : b._value > a._value;
}

inline bool operator<=(const SequenceNumber&  a, const SequenceNumber& b) {
    return (glm::abs(a._value - b._value) < SequenceNumber::THRESHOLD) ? a._value <= b._value : b._value <= a._value;
}

inline bool operator>=(const SequenceNumber&  a, const SequenceNumber& b) {
    return (glm::abs(a._value - b._value) < SequenceNumber::THRESHOLD) ? a._value >= b._value : b._value >= a._value;
}
    
    
inline SequenceNumber operator+(SequenceNumber a, const SequenceNumber::Type& b) {
    a += b;
    return a;
}

inline SequenceNumber operator+(const SequenceNumber::Type& a, SequenceNumber b) {
    b += a;
    return b;
}

inline SequenceNumber operator-(SequenceNumber a, const SequenceNumber::Type& b) {
    a -= b;
    return a;
}

inline SequenceNumber operator-(const SequenceNumber::Type& a, SequenceNumber b) {
    b -= a;
    return b;
}
    
int seqlen(const SequenceNumber& seq1, const SequenceNumber& seq2);
int seqoff(const SequenceNumber& seq1, const SequenceNumber& seq2);
    
}

template<> struct std::hash<udt::SequenceNumber> {
    size_t operator()(const udt::SequenceNumber& SequenceNumber) const {
        return std::hash<unsigned long>()(SequenceNumber._value);
    }
};

#endif // hifi_SequenceNumber_h
