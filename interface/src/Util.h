//
//  Util.h
//  interface/src
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Util_h
#define hifi_Util_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <gpu/Batch.h>

float randFloat();
const glm::vec3 randVector();

void renderWorldBox(gpu::Batch& batch);

void runTimingTests();
void runUnitTests();

bool rayIntersectsSphere(const glm::vec3& rayStarting, const glm::vec3& rayNormalizedDirection,
    const glm::vec3& sphereCenter, float sphereRadius, float& distance);

bool pointInSphere(glm::vec3& point, glm::vec3& sphereCenter, double sphereRadius);

#endif // hifi_Util_h
