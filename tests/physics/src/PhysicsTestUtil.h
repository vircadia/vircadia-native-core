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
#include <BulletUtil.h>

#include <qtextstream.h>

#include <CollisionInfo.h>

const glm::vec3 origin(0.0f);
const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

// Implement these functions for whatever data types you need.
//
// fuzzyCompare takes two args of type T (the type you're comparing), and should
// return an error / difference of type V (eg. if T is a vector, V is a scalar).
// For vector types this is just the distance between a and b.
//
// stringify is just a toString() / repr() style function. For PoD types,
// I'd recommend using the c++11 initialization syntax (type { constructor args... }),
// since it's clear and unambiguous.
//
inline float fuzzyCompare (const glm::vec3 & a, const glm::vec3 & b) {
    return glm::distance(a, b);
}
inline QTextStream & operator << (QTextStream & stream, const glm::vec3 & v) {
    return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }";
}

inline btScalar fuzzyCompare (btScalar a, btScalar b) {
    return a - b;
}

#include "../QTestExtensions.hpp"


#endif // hifi_PhysicsTestUtil_h
