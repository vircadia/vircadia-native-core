//
//  Vec3.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__Vec3__
#define __hifi__Vec3__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QObject>
#include <QString>

/// Scriptable interface a Vec3ernion helper class object. Used exclusively in the JavaScript API
class Vec3 : public QObject {
    Q_OBJECT

public slots:
    glm::vec3 cross(const glm::vec3& v1, const glm::vec3& v2);
    glm::vec3 multiply(const glm::vec3& v1, float f);
    glm::vec3 multiplyQbyV(const glm::quat& q, const glm::vec3& v);
    glm::vec3 sum(const glm::vec3& v1, const glm::vec3& v2);
    glm::vec3 subtract(const glm::vec3& v1, const glm::vec3& v2);
    float length(const glm::vec3& v);
    glm::vec3 normalize(const glm::vec3& v);
    void print(const QString& lable, const glm::vec3& v);
};



#endif /* defined(__hifi__Vec3__) */
