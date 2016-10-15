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
CharacterGhostShape::CharacterGhostShape(const btConvexHullShape* shape) :
        btConvexHullShape(reinterpret_cast<const btScalar*>(shape->getUnscaledPoints()), shape->getNumPoints(), sizeof(btVector3)) {
    assert(shape);
    assert(shape->getUnscaledPoints());
    assert(shape->getNumPoints() > 0);
    setMargin(shape->getMargin());
}

void CharacterGhostShape::getAabb (const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const {
    btConvexHullShape::getAabb(t, aabbMin, aabbMax);
    // double the size of the Aabb by expanding both corners by half the extent
    btVector3 expansion = 0.5f * (aabbMax - aabbMin);
    aabbMin -= expansion;
    aabbMax += expansion;
}
