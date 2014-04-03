//
//  PhysicsTestUtil.cpp
//  physics-tests
//
//  Created by Andrew Meadows on 2014.02.21
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtc/type_ptr.hpp>

#include "PhysicsTestUtil.h"

std::ostream& operator<<(std::ostream& s, const glm::vec3& v) {
    s << "<" << v.x << "," << v.y << "," << v.z << ">";
    return s;
}

std::ostream& operator<<(std::ostream& s, const glm::quat& q) {
    s << "<" << q.x << "," << q.y << "," << q.z << "," << q.w << ">";
    return s;
}

std::ostream& operator<<(std::ostream& s, const glm::mat4& m) {
    s << "[";
    for (int j = 0; j < 4; ++j) {
        s << "  " << m[0][j] << "  " << m[1][j] << "  " << m[2][j] << "  " << m[3][j] << ";";
    }
    s << "  ]";
    return s;
}

std::ostream& operator<<(std::ostream& s, const CollisionInfo& c) {
    s << "[penetration=" << c._penetration 
        << ", contactPoint=" << c._contactPoint
        << ", addedVelocity=" << c._addedVelocity;
    return s;
}
