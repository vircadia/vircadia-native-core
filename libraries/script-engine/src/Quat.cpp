//
//  Quat.cpp
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

#include <OctreeConstants.h>
#include <SharedUtil.h>
#include "Quat.h"


glm::quat Quat::multiply(const glm::quat& q1, const glm::quat& q2) { 
    return q1 * q2; 
}

glm::quat Quat::fromVec3Degrees(const glm::vec3& eulerAngles) { 
    return glm::quat(glm::radians(eulerAngles)); 
}

glm::quat Quat::fromVec3Radians(const glm::vec3& eulerAngles) { 
    return glm::quat(eulerAngles); 
}

glm::quat Quat::fromPitchYawRollDegrees(float pitch, float yaw, float roll) { 
    return glm::quat(glm::radians(glm::vec3(pitch, yaw, roll)));
}

glm::quat Quat::fromPitchYawRollRadians(float pitch, float yaw, float roll) { 
    return glm::quat(glm::vec3(pitch, yaw, roll));
}

glm::quat Quat::inverse(const glm::quat& q) {
    return glm::inverse(q);
}

glm::vec3 Quat::getFront(const glm::quat& orientation) {
    return orientation * IDENTITY_FRONT;
}

glm::vec3 Quat::getRight(const glm::quat& orientation) {
    return orientation * IDENTITY_RIGHT;
}

glm::vec3 Quat::getUp(const glm::quat& orientation) {
    return orientation * IDENTITY_UP;
}

glm::vec3 Quat::safeEulerAngles(const glm::quat& orientation) {
    return glm::degrees(::safeEulerAngles(orientation));
}

glm::quat Quat::angleAxis(float angle, const glm::vec3& v) {
    return glm::angleAxis(glm::radians(angle), v);
}

glm::quat Quat::mix(const glm::quat& q1, const glm::quat& q2, float alpha) {
    return safeMix(q1, q2, alpha);
}

/// Spherical Linear Interpolation
glm::quat Quat::slerp(const glm::quat& q1, const glm::quat& q2, float alpha) {
    return glm::slerp(q1, q2, alpha);
}

// Spherical Quadratic Interpolation
glm::quat Quat::squad(const glm::quat& q1, const glm::quat& q2, const glm::quat& s1, const glm::quat& s2, float h) {
    return glm::squad(q1, q2, s1, s2, h);
}

float Quat::dot(const glm::quat& q1, const glm::quat& q2) {
    return glm::dot(q1, q2);
}

void Quat::print(const QString& lable, const glm::quat& q) {
    qDebug() << qPrintable(lable) << q.x << "," << q.y << "," << q.z << "," << q.w;
}

