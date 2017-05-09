//
//  Quat.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Scriptable Quaternion class library.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Quat_h
#define hifi_Quat_h

#include <glm/gtc/quaternion.hpp>

#include <QObject>
#include <QString>
#include <QtScript/QScriptable>

/**jsdoc
 * A Quaternion
 *
 * @typedef Quat
 * @property {float} x imaginary component i.
 * @property {float} y imaginary component j.
 * @property {float} z imaginary component k.
 * @property {float} w real component.
 */

/// Scriptable interface a Quaternion helper class object. Used exclusively in the JavaScript API
class Quat : public QObject, protected QScriptable {
    Q_OBJECT

public slots:
    glm::quat multiply(const glm::quat& q1, const glm::quat& q2);
    glm::quat normalize(const glm::quat& q);
    glm::quat conjugate(const glm::quat& q);
    glm::quat lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
    glm::quat lookAtSimple(const glm::vec3& eye, const glm::vec3& center);
    glm::quat rotationBetween(const glm::vec3& v1, const glm::vec3& v2);
    glm::quat fromVec3Degrees(const glm::vec3& vec3); // degrees
    glm::quat fromVec3Radians(const glm::vec3& vec3); // radians
    glm::quat fromPitchYawRollDegrees(float pitch, float yaw, float roll); // degrees
    glm::quat fromPitchYawRollRadians(float pitch, float yaw, float roll); // radians
    glm::quat inverse(const glm::quat& q);
    // redundant, calls getForward which better describes the returned vector as a direction
    glm::vec3 getFront(const glm::quat& orientation) { return getForward(orientation); }
    glm::vec3 getForward(const glm::quat& orientation);
    glm::vec3 getRight(const glm::quat& orientation);
    glm::vec3 getUp(const glm::quat& orientation);
    glm::vec3 safeEulerAngles(const glm::quat& orientation); // degrees
    glm::quat angleAxis(float angle, const glm::vec3& v);   // degrees
    glm::vec3 axis(const glm::quat& orientation);
    float angle(const glm::quat& orientation);
    glm::quat mix(const glm::quat& q1, const glm::quat& q2, float alpha);
    glm::quat slerp(const glm::quat& q1, const glm::quat& q2, float alpha);
    glm::quat squad(const glm::quat& q1, const glm::quat& q2, const glm::quat& s1, const glm::quat& s2, float h);
    float dot(const glm::quat& q1, const glm::quat& q2);
    void print(const QString& label, const glm::quat& q, bool asDegrees = false);
    bool equal(const glm::quat& q1, const glm::quat& q2);
    glm::quat cancelOutRollAndPitch(const glm::quat& q);
    glm::quat cancelOutRoll(const glm::quat& q);
};

#endif // hifi_Quat_h
