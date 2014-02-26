//
//  PhysicsTestUtil.h
//  physics-tests
//
//  Created by Andrew Meadows on 2014.02.21
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __tests__PhysicsTestUtil__
#define __tests__PhysicsTestUtil__

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <CollisionInfo.h>

const glm::vec3 xAxis(1.f, 0.f, 0.f);
const glm::vec3 yAxis(0.f, 1.f, 0.f);
const glm::vec3 zAxis(0.f, 0.f, 1.f);

const float rightAngle = 90.f;  // degrees

std::ostream& operator<<(std::ostream& s, const glm::vec3& v);
std::ostream& operator<<(std::ostream& s, const glm::quat& q);
std::ostream& operator<<(std::ostream& s, const glm::mat4& m);
std::ostream& operator<<(std::ostream& s, const CollisionInfo& c);

#endif // __tests__PhysicsTestUtil__
