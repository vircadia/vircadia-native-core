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
#include <QSettings>

float randFloat();
const glm::vec3 randVector();

void renderWorldBox();
int widthText(float scale, int mono, char const* string);

void drawText(int x, int y, float scale, float radians, int mono,
              char const* string, const float* color);

void renderCollisionOverlay(int width, int height, float magnitude, float red = 0, float blue = 0, float green = 0);

void runTimingTests();

bool rayIntersectsSphere(const glm::vec3& rayStarting, const glm::vec3& rayNormalizedDirection,
    const glm::vec3& sphereCenter, float sphereRadius, float& distance);

bool pointInSphere(glm::vec3& point, glm::vec3& sphereCenter, double sphereRadius);

#endif // hifi_Util_h
