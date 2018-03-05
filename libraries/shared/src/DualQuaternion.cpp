//
//  DualQuaternion.cpp
//
//  Created by Anthony J. Thibault on Dec 13th 2017.
//  Copyright (c) 2017 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DualQuaternion.h"
#include "GLMHelpers.h"

// delegating constructor
DualQuaternion::DualQuaternion() : _real(1.0f, 0.0f, 0.0f, 0.0), _dual(0.0f, 0.0f, 0.0f, 0.0f) {
}

DualQuaternion::DualQuaternion(const glm::mat4& m) : DualQuaternion(glmExtractRotation(m), extractTranslation(m)) {
}

DualQuaternion::DualQuaternion(const glm::quat& real, const glm::quat& dual) : _real(real), _dual(dual) {
}

DualQuaternion::DualQuaternion(const glm::vec4& real, const glm::vec4& dual) :
    _real(real.w, real.x, real.y, real.z),
    _dual(dual.w, dual.x, dual.y, dual.z) {
}

DualQuaternion::DualQuaternion(const glm::quat& rotation, const glm::vec3& translation) {
    _real = rotation;
    _dual = glm::quat(0.0f, 0.5f * translation.x, 0.5f * translation.y, 0.5f * translation.z) * rotation;
}

DualQuaternion DualQuaternion::operator*(const DualQuaternion& rhs) const {
    return DualQuaternion(_real * rhs._real, _real * rhs._dual + _dual * rhs._real);
}

DualQuaternion DualQuaternion::operator*(float scalar) const {
    return DualQuaternion(_real * scalar, _dual * scalar);
}

DualQuaternion DualQuaternion::operator+(const DualQuaternion& rhs) const {
    return DualQuaternion(_real + rhs._real, _dual + rhs._dual);
}

glm::vec3 DualQuaternion::xformPoint(const glm::vec3& rhs) const {
    DualQuaternion v(glm::quat(), glm::quat(0.0f, rhs.x, rhs.y, rhs.z));
    DualQuaternion dualConj(glm::conjugate(_real), -glm::conjugate(_dual));
    DualQuaternion result = *this * v * dualConj;
    return vec3(result._dual.x, result._dual.y, result._dual.z);
}

glm::quat DualQuaternion::getRotation() const {
    return _real;
}

glm::vec3 DualQuaternion::getTranslation() const {
    glm::quat result = 2.0f * (_dual * glm::inverse(_real));
    return glm::vec3(result.x, result.y, result.z);
}

glm::vec3 DualQuaternion::xformVector(const glm::vec3& rhs) const {
    return _real * rhs;
}

DualQuaternion DualQuaternion::inverse() const {
    glm::quat invReal = glm::inverse(_real);
    return DualQuaternion(invReal, - invReal * _dual * invReal);
}

DualQuaternion DualQuaternion::conjugate() const {
    return DualQuaternion(glm::conjugate(_real), glm::conjugate(_dual));
}

float DualQuaternion::length() const {
    float dot = this->dot(*this);
    return sqrtf(dot);
}

DualQuaternion DualQuaternion::normalize() const {
    float invLen = 1.0f / length();
    return *this * invLen;
}

float DualQuaternion::dot(const DualQuaternion& rhs) const {
    DualQuaternion result = *this * conjugate();
    return result._real.w;
}

DualQuaternion DualQuaternion::operator-() const {
    return DualQuaternion(-_real, -_dual);
}
