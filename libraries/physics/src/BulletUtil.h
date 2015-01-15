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

#endif // hifi_BulletUtil_h
