//
//  Vec3.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/vector_angle.hpp>

#include <QDebug>

#include <GLMHelpers.h>

#include "ScriptEngineLogging.h"
#include "NumericalConstants.h"
#include "Vec3.h"


float Vec3::orientedAngle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
    float radians = glm::orientedAngle(glm::normalize(v1), glm::normalize(v2), glm::normalize(v3));
    return glm::degrees(radians);
}


void Vec3::print(const QString& lable, const glm::vec3& v) {
    qCDebug(scriptengine) << qPrintable(lable) << v.x << "," << v.y << "," << v.z;
}

bool Vec3::withinEpsilon(const glm::vec3& v1, const glm::vec3& v2, float epsilon) {
    float distanceSquared = glm::length2(v1 - v2);
    return (epsilon*epsilon) >= distanceSquared;
}

glm::vec3 Vec3::toPolar(const glm::vec3& v) {
    float radius = length(v);
    if (glm::abs(radius) < EPSILON) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    glm::vec3 u = v / radius;
    
    float elevation, azimuth;
    
    elevation = glm::asin(-u.y);
    azimuth = atan2(v.x, v.z);
    
    // Round off small decimal values
    if (glm::abs(elevation) < EPSILON) {
        elevation = 0.0f;
    }
    if (glm::abs(azimuth) < EPSILON) {
        azimuth = 0.0f;
    }

    return glm::vec3(elevation, azimuth, radius);
}

glm::vec3 Vec3::fromPolar(const glm::vec3& polar) {
    float x = glm::cos(polar.x) * glm::sin(polar.y);
    float y = glm::sin(-polar.x);
    float z = glm::cos(polar.x) * glm::cos(polar.y);

    // Round small values to 0
    if (glm::abs(x) < EPSILON) {
        x = 0.0f;
    }
    if (glm::abs(y) < EPSILON) {
        y = 0.0f;
    }
    if (glm::abs(z) < EPSILON) {
        z = 0.0f;
    }

    return polar.z * glm::vec3(x, y, z);
}

glm::vec3 Vec3::fromPolar(float elevation, float azimuth) {
    glm::vec3 v = glm::vec3(elevation, azimuth, 1.0f);
    return fromPolar(v);
}

float Vec3::angle(const glm::vec3& fromV, const glm::vec3& toV) {
	glm::vec3 fromVNormalized = glm::normalize(fromV);
	glm::vec3 toVNormalized = glm::normalize(toV);

	float radians = glm::acos(glm::dot(fromVNormalized, toVNormalized));

	return radians * 180.0f / PI;
}

glm::vec3 Vec3::lerp(const glm::vec3& v1, const glm::vec3& v2, float t) {
	return v1 * (1.0f - t) + v2 * t;
}
