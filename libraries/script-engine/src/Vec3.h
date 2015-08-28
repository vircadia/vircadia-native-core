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

#ifndef hifi_Vec3_h
#define hifi_Vec3_h
 
#include <QObject>
#include <QString>

#include "GLMHelpers.h"

/// Scriptable interface a Vec3ernion helper class object. Used exclusively in the JavaScript API
class Vec3 : public QObject {
    Q_OBJECT

public slots:
    vec3 reflect(const vec3& v1, const vec3& v2);
    vec3 cross(const vec3& v1, const vec3& v2);
    float dot(const vec3& v1, const vec3& v2);
    vec3 multiply(const vec3& v1, float f);
    vec3 multiply(float, const vec3& v1);
    vec3 multiplyQbyV(const glm::quat& q, const vec3& v);
    vec3 sum(const vec3& v1, const vec3& v2);
    vec3 subtract(const vec3& v1, const vec3& v2);
    float length(const vec3& v);
    float distance(const vec3& v1, const vec3& v2);
    float orientedAngle(const vec3& v1, const vec3& v2, const vec3& v3);
    vec3 normalize(const vec3& v);
    vec3 mix(const vec3& v1, const vec3& v2, float m);
    void print(const QString& lable, const vec3& v);
    bool equal(const vec3& v1, const vec3& v2);
    vec3 toPolar(const vec3& v);
    vec3 fromPolar(const vec3& polar);
    vec3 fromPolar(float elevation, float azimuth);
    const vec3& UNIT_X();
    const vec3& UNIT_Y();
    const vec3& UNIT_Z();
    const vec3& UNIT_NEG_X();
    const vec3& UNIT_NEG_Y();
    const vec3& UNIT_NEG_Z();
    const vec3& UNIT_XY();
    const vec3& UNIT_XZ();
    const vec3& UNIT_YZ();
    const vec3& UNIT_XYZ();
    const vec3& FLOAT_MAX();
    const vec3& FLOAT_MIN();
    const vec3& ZERO();
    const vec3& ONE();
    const vec3& RIGHT();
    const vec3& UP();
    const vec3& FRONT();
};



#endif // hifi_Vec3_h
