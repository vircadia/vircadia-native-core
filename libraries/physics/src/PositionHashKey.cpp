//
//  PositionHashKey.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>

#include "PositionHashKey.h"

// static 
int computeHash(const glm::vec3& center) {
    // NOTE: 0.49f is used to bump the float up almost half a millimeter
    // so the cast to int produces a round() effect rather than a floor()
    int hash = DoubleHashKey::hashFunction((int)(center.x * MILLIMETERS_PER_METER + copysignf(1.0f, center.x) * 0.49f), 0);
    hash ^= DoubleHashKey::hashFunction((int)(center.y * MILLIMETERS_PER_METER + copysignf(1.0f, center.y) * 0.49f), 1);
    return hash ^ DoubleHashKey::hashFunction((int)(center.z * MILLIMETERS_PER_METER + copysignf(1.0f, center.z) * 0.49f), 2);
}

// static 
int computeHash2(const glm::vec3& center) {
    // NOTE: 0.49f is used to bump the float up almost half a millimeter
    // so the cast to int produces a round() effect rather than a floor()
    int hash = DoubleHashKey::hashFunction2((int)(center.x * MILLIMETERS_PER_METER + copysignf(1.0f, center.x) * 0.49f));
    hash ^= DoubleHashKey::hashFunction2((int)(center.y * MILLIMETERS_PER_METER + copysignf(1.0f, center.y) * 0.49f));
    return hash ^ DoubleHashKey::hashFunction2((int)(center.z * MILLIMETERS_PER_METER + copysignf(1.0f, center.z) * 0.49f));
}

PositionHashKey::PositionHashKey(glm::vec3 center) : DoubleHashKey() {
    _hash = computeHash(center);
    _hash2 = computeHash2(center);
}
