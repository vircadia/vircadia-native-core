//
//  PhysicsTestUtil.cpp
//  physics-tests
//
//  Created by Andrew Meadows on 2014.02.21
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "PhysicsTestUtil.h"

std::ostream& operator<<(std::ostream& s, const glm::vec3& v) {
    s << "<" << v.x << "," << v.y << "," << v.z << ">";
    return s;
}

std::ostream& operator<<(std::ostream& s, const CollisionInfo& c) {
    s << "[penetration=" << c._penetration 
        << ", contactPoint=" << c._contactPoint
        << ", addedVelocity=" << c._addedVelocity;
    return s;
}
