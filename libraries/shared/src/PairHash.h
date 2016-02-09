//
//  PairHash.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2016-02-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_PairHash_h
#define hifi_PairHash_h

// this header adds struct pair_hash in order to handle the use of an std::pair as the key of an unordered_map

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

// Only for pairs of std::hash-able types for simplicity.
// You can of course template this struct to allow other hash functions
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        std::size_t seed = 0;

        hash_combine(seed, p.first);
        hash_combine(seed, p.second);

        return seed;
    }
};

#endif // hifi_PairHash_h
