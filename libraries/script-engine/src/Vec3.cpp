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

#include "ScriptEngineLogging.h"
#include "NumericalConstants.h"
#include "Vec3.h"

glm::vec3 Vec3::reflect(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::reflect(v1, v2);
}
glm::vec3 Vec3::cross(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::cross(v1,v2);
}

float Vec3::dot(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::dot(v1,v2);
}

glm::vec3 Vec3::multiply(const glm::vec3& v1, float f) {
    return v1 * f;
}

glm::vec3 Vec3::multiply(float f, const glm::vec3& v1) {
    return v1 * f;
}

glm::vec3 Vec3::multiplyQbyV(const glm::quat& q, const glm::vec3& v) {
    return q * v;
}

glm::vec3 Vec3::sum(const glm::vec3& v1, const glm::vec3& v2) { 
    return v1 + v2;
}
glm::vec3 Vec3::subtract(const glm::vec3& v1, const glm::vec3& v2) {
    return v1 - v2;
}

float Vec3::length(const glm::vec3& v) {
    return glm::length(v);
}

float Vec3::distance(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::distance(v1, v2);
}

float Vec3::orientedAngle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
    return glm::degrees(glm::orientedAngle(glm::normalize(v1), glm::normalize(v2), glm::normalize(v3)));
}

glm::vec3 Vec3::normalize(const glm::vec3& v) {
    return glm::normalize(v);
}

glm::vec3 Vec3::mix(const glm::vec3& v1, const glm::vec3& v2, float m) {
    return glm::mix(v1, v2, m);
}

void Vec3::print(const QString& lable, const glm::vec3& v) {
    qCDebug(scriptengine) << qPrintable(lable) << v.x << "," << v.y << "," << v.z;
}

bool Vec3::equal(const glm::vec3& v1, const glm::vec3& v2) {
    return v1 == v2;
}

glm::vec3 Vec3::toPolar(const glm::vec3& v) {
    glm::vec3 u = normalize(v);
    float pitch, yaw, temp;
    pitch = glm::asin(-u.y);
    temp = glm::cos(pitch);
    if (glm::abs(pitch) >= (PI - EPSILON)) {
        yaw = 0.0;
        if (pitch > 0) {
            pitch = PI_OVER_TWO;
        } else {
            pitch = -PI_OVER_TWO;
        }
    } else {
        if (u.z < 0.0) {
            if (u.x > 0.0 && u.x < 1.0) {
                yaw = PI - glm::asin(u.x / temp);
            } else {
                yaw = -PI - glm::asin(u.x / temp);
            }
        } else {
            yaw = glm::asin(u.x / temp);
        }
    }
    
    // Round small values to 0
    if (glm::abs(pitch) < EPSILON) {
        pitch = 0.0;
    }
    if (glm::abs(yaw) < EPSILON) {
        yaw = 0.0;
    }
    
    // Neglect roll component
    return glm::vec3(glm::degrees(pitch), glm::degrees(yaw), 0.0);
}

glm::vec3 Vec3::fromPolar(float pitch, float yaw) {
    pitch = glm::radians(pitch);
    yaw = glm::radians(yaw);
    
    float x = glm::cos(pitch) * glm::sin(yaw);
    float y = glm::sin(-pitch);
    float z = glm::cos(pitch) * glm::cos(yaw);
    
    // Round small values to 0
    if (glm::abs(x) < EPSILON) {
        x = 0.0;
    }
    if (glm::abs(y) < EPSILON) {
        y = 0.0;
    }
    if (glm::abs(z) < EPSILON) {
        z = 0.0;
    }
    
    return glm::vec3(x, y, z);
}



