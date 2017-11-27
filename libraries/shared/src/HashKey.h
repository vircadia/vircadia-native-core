//
//  HashKey.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2017.10.25
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HashKey_h
#define hifi_HashKey_h

#include <cstddef>
#include <stdint.h>

#include <glm/glm.hpp>


// HashKey for use with btHashMap which requires a particular API for its keys.  In particular it
// requires its Key to implement these methods:
//
//      bool Key::equals()
//      int32_t Key::getHash()
//
// The important thing about the HashKey implementation is that while getHash() returns 32-bits,
// internally HashKey stores a 64-bit hash which is used for the equals() comparison.  This allows
// btHashMap to insert "dupe" 32-bit keys to different "values".

class HashKey {
public:
    static float getNumQuantizedValuesPerMeter();

    // These two methods are required by btHashMap.
    bool equals(const HashKey& other) const { return _hash == other._hash; }
    int32_t getHash() const { return (int32_t)((uint32_t)_hash); }

    void clear() { _hash = _hashCount = 0; }
    bool isNull() const { return _hash == 0 && _hashCount == 0; }
    void hashUint64(uint64_t data);
    void hashFloat(float data);
    void hashVec3(const glm::vec3& data);

    uint64_t getHash64() const { return _hash; } // for debug/test purposes

private:
    uint64_t _hash { 0 };
    uint8_t _hashCount { 0 };
};

#endif // hifi_HashKey_h
