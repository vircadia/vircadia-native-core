//
//  PhysicsTestUtil.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 02/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtc/type_ptr.hpp>

#include "PhysicsTestUtil.h"
#include "StreamUtils.h"

std::ostream& operator<<(std::ostream& s, const CollisionInfo& c) {
    s << "[penetration=" << c._penetration 
        << ", contactPoint=" << c._contactPoint
        << ", addedVelocity=" << c._addedVelocity;
    return s;
}
