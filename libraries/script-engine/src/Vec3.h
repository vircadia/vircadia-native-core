//
//  Vec3.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Scriptable Vec3 class library.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Vec3_h
#define hifi_Vec3_h

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GLMHelpers.h"

/**jsdoc
 * A 2-dimensional vector.
 *
 * @typedef Vec2
 * @property {float} x X-coordinate of the vector.
 * @property {float} y Y-coordinate of the vector.
 */

/**jsdoc
 * A 3-dimensional vector.
 *
 * @typedef Vec3
 * @property {float} x X-coordinate of the vector.
 * @property {float} y Y-coordinate of the vector.
 * @property {float} z Z-coordinate of the vector.
 */


/// Scriptable interface a Vec3ernion helper class object. Used exclusively in the JavaScript API
class Vec3 : public QObject {
    Q_OBJECT
    Q_PROPERTY(glm::vec3 UNIT_X READ UNIT_X CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_Y READ UNIT_Y CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_Z READ UNIT_Z CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_NEG_X READ UNIT_NEG_X CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_NEG_Y READ UNIT_NEG_Y CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_NEG_Z READ UNIT_NEG_Z CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_XY READ UNIT_XY CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_XZ READ UNIT_XZ CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_YZ READ UNIT_YZ CONSTANT)
    Q_PROPERTY(glm::vec3 UNIT_XYZ READ UNIT_XYZ CONSTANT)
    Q_PROPERTY(glm::vec3 FLOAT_MAX READ FLOAT_MAX CONSTANT)
    Q_PROPERTY(glm::vec3 FLOAT_MIN READ FLOAT_MIN CONSTANT)
    Q_PROPERTY(glm::vec3 ZERO READ ZERO CONSTANT)
    Q_PROPERTY(glm::vec3 ONE READ ONE CONSTANT)
    Q_PROPERTY(glm::vec3 TWO READ TWO CONSTANT)
    Q_PROPERTY(glm::vec3 HALF READ HALF CONSTANT)
    Q_PROPERTY(glm::vec3 RIGHT READ RIGHT CONSTANT)
    Q_PROPERTY(glm::vec3 UP READ UP CONSTANT)
    Q_PROPERTY(glm::vec3 FRONT READ FRONT CONSTANT)

public slots:
    glm::vec3 reflect(const glm::vec3& v1, const glm::vec3& v2) { return glm::reflect(v1, v2); }
    glm::vec3 cross(const glm::vec3& v1, const glm::vec3& v2) { return glm::cross(v1, v2); }
    float dot(const glm::vec3& v1, const glm::vec3& v2) { return glm::dot(v1, v2); }
    glm::vec3 multiply(const glm::vec3& v1, float f) { return v1 * f; }
    glm::vec3 multiply(float f, const glm::vec3& v1) { return v1 * f; }
    glm::vec3 multiplyVbyV(const glm::vec3& v1, const glm::vec3& v2) { return v1 * v2; }
    glm::vec3 multiplyQbyV(const glm::quat& q, const glm::vec3& v) { return q * v; }
    glm::vec3 sum(const glm::vec3& v1, const glm::vec3& v2) { return v1 + v2; }
    glm::vec3 subtract(const glm::vec3& v1, const glm::vec3& v2) { return v1 - v2; }
    float length(const glm::vec3& v) { return glm::length(v); }
    float distance(const glm::vec3& v1, const glm::vec3& v2) { return glm::distance(v1, v2); }
    float orientedAngle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3);
    glm::vec3 normalize(const glm::vec3& v) { return glm::normalize(v); };
    glm::vec3 mix(const glm::vec3& v1, const glm::vec3& v2, float m) { return glm::mix(v1, v2, m); }
    void print(const QString& label, const glm::vec3& v);
    bool equal(const glm::vec3& v1, const glm::vec3& v2) { return v1 == v2; }
    bool withinEpsilon(const glm::vec3& v1, const glm::vec3& v2, float epsilon);
    // FIXME misnamed, should be 'spherical' or 'euler' depending on the implementation
    glm::vec3 toPolar(const glm::vec3& v);
    glm::vec3 fromPolar(const glm::vec3& polar);
    glm::vec3 fromPolar(float elevation, float azimuth);

private:
    const glm::vec3& UNIT_X() { return Vectors::UNIT_X; }
    const glm::vec3& UNIT_Y() { return Vectors::UNIT_Y; }
    const glm::vec3& UNIT_Z() { return Vectors::UNIT_Z; }
    const glm::vec3& UNIT_NEG_X() { return Vectors::UNIT_NEG_X; }
    const glm::vec3& UNIT_NEG_Y() { return Vectors::UNIT_NEG_Y; }
    const glm::vec3& UNIT_NEG_Z() { return Vectors::UNIT_NEG_Z; }
    const glm::vec3& UNIT_XY() { return Vectors::UNIT_XY; }
    const glm::vec3& UNIT_XZ() { return Vectors::UNIT_XZ; }
    const glm::vec3& UNIT_YZ() { return Vectors::UNIT_YZ; }
    const glm::vec3& UNIT_XYZ() { return Vectors::UNIT_XYZ; }
    const glm::vec3& FLOAT_MAX() { return Vectors::MAX; }
    const glm::vec3& FLOAT_MIN() { return Vectors::MIN; }
    const glm::vec3& ZERO() { return Vectors::ZERO; }
    const glm::vec3& ONE() { return Vectors::ONE; }
    const glm::vec3& TWO() { return Vectors::TWO; }
    const glm::vec3& HALF() { return Vectors::HALF; }
    const glm::vec3& RIGHT() { return Vectors::RIGHT; }
    const glm::vec3& UP() { return Vectors::UP; }
    const glm::vec3& FRONT() { return Vectors::FRONT; }
};

#endif // hifi_Vec3_h
