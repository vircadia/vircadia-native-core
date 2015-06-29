//
//  GlmTestUtils.h
//  tests/physics/src
//
//  Created by Seiji Emery on 6/22/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GlmTestUtils_h
#define hifi_GlmTestUtils_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

// Implements functionality in QTestExtensions.h for glm types

inline float getErrorDifference(const glm::vec3 & a, const glm::vec3 & b) {
    return glm::distance(a, b);
}
inline QTextStream & operator << (QTextStream & stream, const glm::vec3 & v) {
    return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }";
}

#endif
