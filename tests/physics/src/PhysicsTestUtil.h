//
//  PhysicsTestUtil.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 02/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsTestUtil_h
#define hifi_PhysicsTestUtil_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <CollisionInfo.h>

const glm::vec3 xAxis(1.f, 0.f, 0.f);
const glm::vec3 yAxis(0.f, 1.f, 0.f);
const glm::vec3 zAxis(0.f, 0.f, 1.f);

std::ostream& operator<<(std::ostream& s, const CollisionInfo& c);

#endif // hifi_PhysicsTestUtil_h
