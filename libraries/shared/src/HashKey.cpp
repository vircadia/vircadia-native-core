//
//  HashKey.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2017.10.25
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HashKey.h"

#include "NumericalConstants.h"


const uint8_t NUM_PRIMES = 64;
const uint64_t PRIMES[] = {
    4194301UL, 4194287UL, 4194277UL, 4194271UL, 4194247UL, 4194217UL, 4194199UL, 4194191UL,
    4194187UL, 4194181UL, 4194173UL, 4194167UL, 4194143UL, 4194137UL, 4194131UL, 4194107UL,
    4194103UL, 4194023UL, 4194011UL, 4194007UL, 4193977UL, 4193971UL, 4193963UL, 4193957UL,
    4193939UL, 4193929UL, 4193909UL, 4193869UL, 4193807UL, 4193803UL, 4193801UL, 4193789UL,
    4193759UL, 4193753UL, 4193743UL, 4193701UL, 4193663UL, 4193633UL, 4193573UL, 4193569UL,
    4193551UL, 4193549UL, 4193531UL, 4193513UL, 4193507UL, 4193459UL, 4193447UL, 4193443UL,
    4193417UL, 4193411UL, 4193393UL, 4193389UL, 4193381UL, 4193377UL, 4193369UL, 4193359UL,
    4193353UL, 4193327UL, 4193309UL, 4193303UL, 4193297UL, 4193279UL, 4193269UL, 4193263UL
};


// this hash function inspired by Squirrel Eiserloh's GDC2017 talk: "Noise-Based RNG"
uint64_t squirrel3_64(uint64_t data, uint8_t primeIndex) {
    constexpr uint64_t BIT_NOISE1 = 2760725261486592643UL;
    constexpr uint64_t BIT_NOISE2 = 6774464464027632833UL;
    constexpr uint64_t BIT_NOISE3 = 5545331650366059883UL;

    // blend prime numbers into the hash to prevent dupes
    // when hashing the same set of numbers in a different order
    uint64_t hash = PRIMES[primeIndex % NUM_PRIMES] * data;
    hash *= BIT_NOISE1;
    hash ^= (hash >> 16);
    hash += BIT_NOISE2;
    hash ^= (hash << 16);
    hash *= BIT_NOISE3;
    return hash ^ (hash >> 16);
}

constexpr float QUANTIZED_VALUES_PER_METER = 250.0f;

// static
float HashKey::getNumQuantizedValuesPerMeter() {
    return QUANTIZED_VALUES_PER_METER;
}

void HashKey::hashUint64(uint64_t data) {
    _hash += squirrel3_64(data, ++_hashCount);
}

void HashKey::hashFloat(float data) {
    _hash += squirrel3_64((uint64_t)((int64_t)(data * QUANTIZED_VALUES_PER_METER)), ++_hashCount);
}

void HashKey::hashVec3(const glm::vec3& data) {
    _hash += squirrel3_64((uint64_t)((int64_t)(data[0] * QUANTIZED_VALUES_PER_METER)), ++_hashCount);
    _hash *= squirrel3_64((uint64_t)((int64_t)(data[1] * QUANTIZED_VALUES_PER_METER)), ++_hashCount);
    _hash ^= squirrel3_64((uint64_t)((int64_t)(data[2] * QUANTIZED_VALUES_PER_METER)), ++_hashCount);
}

