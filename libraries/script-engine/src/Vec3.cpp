//
//  Vec3.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable Vec3 class library.
//
//

#include <QDebug>

#include "Vec3.h"

glm::vec3 Vec3::cross(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::cross(v1,v2);
}

glm::vec3 Vec3::multiply(const glm::vec3& v1, float f) {
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

glm::vec3 Vec3::normalize(const glm::vec3& v) {
    return glm::normalize(v);
}

void Vec3::print(const QString& lable, const glm::vec3& v) {
    qDebug() << qPrintable(lable) << v.x << "," << v.y << "," << v.z;
}
