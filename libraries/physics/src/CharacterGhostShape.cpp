//
//  CharacterGhostShape.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2016.09.14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CharacterGhostShape.h"

void CharacterGhostShape::getAabb (const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const {
    btCapsuleShape::getAabb(t, aabbMin, aabbMax);
    // double the size of the Aabb by expanding both corners by half the extent
    btVector3 expansion = 0.5f * (aabbMax - aabbMin);
    aabbMin -= expansion;
    aabbMax += expansion;
}
