//
//  Quat.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable Quaternion class library.
//
//

#include <glm/gtx/vector_angle.hpp>

#include <QDebug>

#include <OctreeConstants.h>
#include <SharedUtil.h>
#include "Quat.h"


glm::quat Quat::multiply(const glm::quat& q1, const glm::quat& q2) { 
    return q1 * q2; 
}

glm::quat Quat::fromVec3(const glm::vec3& vec3) { 
    return glm::quat(vec3); 
}

glm::quat Quat::fromPitchYawRoll(float pitch, float yaw, float roll) { 
    return glm::quat(glm::radians(glm::vec3(pitch, yaw, roll)));
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
    return ::safeEulerAngles(orientation);
}

glm::quat Quat::angleAxis(float angle, const glm::vec3& v) {
    return glm::angleAxis(angle, v);
}

glm::quat Quat::mix(const glm::quat& q1, const glm::quat& q2, float alpha) {
    return safeMix(q1, q2, alpha);
}

void Quat::print(const QString& lable, const glm::quat& q) {
    qDebug() << qPrintable(lable) << q.x << "," << q.y << "," << q.z << "," << q.w;
}

