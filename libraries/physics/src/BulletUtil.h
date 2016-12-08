//
//  BulletUtil.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BulletUtil_h
#define hifi_BulletUtil_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline glm::vec3 bulletToGLM(const btVector3& b) {
    return glm::vec3(b.getX(), b.getY(), b.getZ());
}

inline glm::quat bulletToGLM(const btQuaternion& b) {
    return glm::quat(b.getW(), b.getX(), b.getY(), b.getZ());
}

inline btVector3 glmToBullet(const glm::vec3& g) {
    return btVector3(g.x, g.y, g.z);
}

inline btQuaternion glmToBullet(const glm::quat& g) {
    return btQuaternion(g.x, g.y, g.z, g.w);
}

inline btMatrix3x3 glmToBullet(const glm::mat3& m) {
    return btMatrix3x3(m[0][0], m[1][0], m[2][0],
                       m[0][1], m[1][1], m[2][1],
                       m[0][2], m[1][2], m[2][2]);
}

// btTransform does not contain a full 4x4 matrix, so this transform is lossy.
// Affine transformations are OK but perspective transformations are not.
inline btTransform glmToBullet(const glm::mat4& m) {
    glm::mat3 m3(m);
    return btTransform(glmToBullet(m3), glmToBullet(glm::vec3(m[3][0], m[3][1], m[3][2])));
}

inline glm::mat4 bulletToGLM(const btTransform& t) {
    glm::mat4 m;

    const btMatrix3x3& basis = t.getBasis();
    // copy over 3x3 part
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            m[c][r] = basis[r][c];
        }
    }

    // copy traslation part
    btVector3 origin = t.getOrigin();
    m[3][0] = origin.getX();
    m[3][1] = origin.getY();
    m[3][2] = origin.getZ();

    // set last row
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;
    return m;
}

inline btVector3 rotateVector(const btQuaternion& q, const btVector3& vec) {
    return glmToBullet(bulletToGLM(q) * bulletToGLM(vec));
}

inline btVector3 clampLength(const btVector3& v, const float& maxLength) {
    if (v.length() > maxLength) {
        return v.normalized() * maxLength;
    } else {
        return v;
    }
}

#endif // hifi_BulletUtil_h
