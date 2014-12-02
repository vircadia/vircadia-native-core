//
//  DoubleHashKey.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DoubleHashKey_h
#define hifi_DoubleHashKey_h

// DoubleHashKey for use with btHashMap
class DoubleHashKey {
public:
    static unsigned int hashFunction(unsigned int value, int primeIndex);
    static unsigned int hashFunction2(unsigned int value);

    DoubleHashKey() : _hash(0), _hash2(0) { }

    DoubleHashKey(unsigned int value, int primeIndex = 0) : 
        _hash(hashFunction(value, primeIndex)), 
        _hash2(hashFunction2(value)) { 
    }

    bool equals(const DoubleHashKey& other) const {
        return _hash == other._hash && _hash2 == other._hash2;
    }

    unsigned int getHash() const { return (unsigned int)_hash; }

    int _hash;
    int _hash2;
};

#endif // hifi_DoubleHashKey_h
