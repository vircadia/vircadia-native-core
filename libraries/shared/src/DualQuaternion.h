//
//  DualQuaternion.h
//
//  Created by Anthony J. Thibault on Dec 13th 2017.
//  Copyright (c) 2017 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_DualQuaternion
#define hifi_DualQuaternion

#include <QtGlobal>
#include <QDebug>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class DualQuaternion {
public:
    DualQuaternion();
    explicit DualQuaternion(const glm::mat4& m);
    DualQuaternion(const glm::quat& real, const glm::quat& imag);
    DualQuaternion(const glm::quat& rotation, const glm::vec3& translation);
    DualQuaternion(const glm::vec4& real, const glm::vec4& imag);
    DualQuaternion operator*(const DualQuaternion& rhs) const;
    DualQuaternion operator*(float scalar) const;
    DualQuaternion operator+(const DualQuaternion& rhs) const;

    const glm::quat& real() const { return _real; }
    glm::quat& real() { return _real; }

    const glm::quat& imag() const { return _imag; }
    glm::quat& imag() { return _imag; }

    glm::quat getRotation() const;
    glm::vec3 getTranslation() const;

    glm::vec3 xformPoint(const glm::vec3& rhs) const;
    glm::vec3 xformVector(const glm::vec3& rhs) const;

    DualQuaternion inverse() const;
    DualQuaternion conjugate() const;
    float length() const;
    DualQuaternion normalize() const;

protected:
    friend QDebug operator<<(QDebug debug, const DualQuaternion& pose);
    glm::quat _real;
    glm::quat _imag;
};

inline QDebug operator<<(QDebug debug, const DualQuaternion& dq) {
    debug << "AnimPose, real = (" << dq._real.x << dq._real.y << dq._real.z << dq._real.w << "), imag = (" << dq._imag.x << dq._imag.y << dq._imag.z << dq._imag.w << ")";
    return debug;
}

#endif
