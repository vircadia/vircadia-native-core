//
//  DoubleHashKey.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DoubleHashKey.h"

const uint32_t NUM_PRIMES = 64;
const uint32_t PRIMES[] = {
    4194301U, 4194287U, 4194277U, 4194271U, 4194247U, 4194217U, 4194199U, 4194191U,
    4194187U, 4194181U, 4194173U, 4194167U, 4194143U, 4194137U, 4194131U, 4194107U,
    4194103U, 4194023U, 4194011U, 4194007U, 4193977U, 4193971U, 4193963U, 4193957U,
    4193939U, 4193929U, 4193909U, 4193869U, 4193807U, 4193803U, 4193801U, 4193789U,
    4193759U, 4193753U, 4193743U, 4193701U, 4193663U, 4193633U, 4193573U, 4193569U,
    4193551U, 4193549U, 4193531U, 4193513U, 4193507U, 4193459U, 4193447U, 4193443U,
    4193417U, 4193411U, 4193393U, 4193389U, 4193381U, 4193377U, 4193369U, 4193359U,
    4193353U, 4193327U, 4193309U, 4193303U, 4193297U, 4193279U, 4193269U, 4193263U
};

uint32_t DoubleHashKey::hashFunction(uint32_t value, uint32_t primeIndex) {
    uint32_t hash = PRIMES[primeIndex % NUM_PRIMES] * (value + 1U);
    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3);
    hash ^=  (hash >> 6);
    hash += ~(hash << 11);
    return hash ^ (hash >> 16);
}

uint32_t DoubleHashKey::hashFunction2(uint32_t value) {
    uint32_t hash = 0x811c9dc5U;
    for (uint32_t i = 0; i < 4; i++ ) {
        uint32_t byte = (value << (i * 8)) >> (24 - i * 8);
        hash = ( hash ^ byte ) * 0x01000193U;
    }
   return hash;
}

void DoubleHashKey::computeHash(uint32_t value, uint32_t primeIndex) {
    _hash = DoubleHashKey::hashFunction(value, primeIndex);
    _hash2 = DoubleHashKey::hashFunction2(value);
}
