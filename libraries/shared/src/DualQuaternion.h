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

    const glm::quat& dual() const { return _dual; }
    glm::quat& dual() { return _dual; }

    glm::quat getRotation() const;
    glm::vec3 getTranslation() const;

    glm::vec3 xformPoint(const glm::vec3& rhs) const;
    glm::vec3 xformVector(const glm::vec3& rhs) const;

    DualQuaternion inverse() const;
    DualQuaternion conjugate() const;
    float length() const;
    DualQuaternion normalize() const;
    float dot(const DualQuaternion& rhs) const;
    DualQuaternion operator-() const;

protected:
    friend QDebug operator<<(QDebug debug, const DualQuaternion& pose);
    glm::quat _real;
    glm::quat _dual;
};


inline QDebug operator<<(QDebug debug, const DualQuaternion& dq) {
    debug << "DualQuaternion, real = (" << dq._real.x << dq._real.y << dq._real.z << dq._real.w << "), dual = (" << dq._dual.x << dq._dual.y << dq._dual.z << dq._dual.w << ")";
    return debug;
}

#endif
