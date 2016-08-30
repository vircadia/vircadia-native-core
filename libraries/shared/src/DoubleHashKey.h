//
//  DoubleHashKey.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DoubleHashKey_h
#define hifi_DoubleHashKey_h

#include <stdint.h>

// DoubleHashKey for use with btHashMap
class DoubleHashKey {
public:
    static uint32_t hashFunction(uint32_t value, uint32_t primeIndex);
    static uint32_t hashFunction2(uint32_t value);

    DoubleHashKey() : _hash(0), _hash2(0) { }

    DoubleHashKey(uint32_t value, uint32_t primeIndex = 0) :
        _hash(hashFunction(value, primeIndex)),
        _hash2(hashFunction2(value)) {
    }

    void clear() { _hash = 0; _hash2 = 0; }
    bool isNull() const { return _hash == 0 && _hash2 == 0; }

    bool equals(const DoubleHashKey& other) const {
        return _hash == other._hash && _hash2 == other._hash2;
    }

    void computeHash(uint32_t value, uint32_t primeIndex = 0);
    uint32_t getHash() const { return _hash; }
    uint32_t getHash2() const { return _hash2; }

    void setHash(uint32_t hash) { _hash = hash; }
    void setHash2(uint32_t hash2) { _hash2 = hash2; }

private:
    uint32_t _hash;
    uint32_t _hash2;
};

#endif // hifi_DoubleHashKey_h
