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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QObject>
#include <QString>

/// Scriptable interface a Vec3ernion helper class object. Used exclusively in the JavaScript API
class Vec3 : public QObject {
    Q_OBJECT

public slots:
    glm::vec3 cross(const glm::vec3& v1, const glm::vec3& v2);
    float dot(const glm::vec3& v1, const glm::vec3& v2);
    glm::vec3 multiply(const glm::vec3& v1, float f);
    glm::vec3 multiply(float, const glm::vec3& v1);
    glm::vec3 multiplyQbyV(const glm::quat& q, const glm::vec3& v);
    glm::vec3 sum(const glm::vec3& v1, const glm::vec3& v2);
    glm::vec3 subtract(const glm::vec3& v1, const glm::vec3& v2);
    float length(const glm::vec3& v);
    float distance(const glm::vec3& v1, const glm::vec3& v2);
    float orientedAngle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3);
    glm::vec3 normalize(const glm::vec3& v);
    glm::vec3 mix(const glm::vec3& v1, const glm::vec3& v2, float m);
    void print(const QString& lable, const glm::vec3& v);
    bool equal(const glm::vec3& v1, const glm::vec3& v2);
};



#endif // hifi_Vec3_h
