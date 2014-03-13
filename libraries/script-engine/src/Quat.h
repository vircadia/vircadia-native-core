//
//  Quat.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable Quaternion class library.
//
//

#ifndef __hifi__Quat__
#define __hifi__Quat__

#include <glm/gtc/quaternion.hpp>

#include <QObject>
#include <QString>

/// Scriptable interface a Quaternion helper class object. Used exclusively in the JavaScript API
class Quat : public QObject {
    Q_OBJECT

public slots:
    glm::quat multiply(const glm::quat& q1, const glm::quat& q2);
    glm::quat fromVec3(const glm::vec3& vec3);
    glm::quat fromPitchYawRoll(float pitch, float yaw, float roll); // degrees
    glm::quat inverse(const glm::quat& q);
    glm::vec3 getFront(const glm::quat& orientation);
    glm::vec3 getRight(const glm::quat& orientation);
    glm::vec3 getUp(const glm::quat& orientation);
    glm::vec3 safeEulerAngles(const glm::quat& orientation); // degrees
    glm::quat angleAxis(float angle, const glm::vec3& v);   // degrees
    glm::quat mix(const glm::quat& q1, const glm::quat& q2, float alpha);
    void print(const QString& lable, const glm::quat& q);
};

#endif /* defined(__hifi__Quat__) */
