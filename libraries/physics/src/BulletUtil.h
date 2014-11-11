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

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline void bulletToGLM(const btVector3& b, glm::vec3& g) { 
    g = glm::vec3(b.getX(), b.getY(), b.getZ());
}

inline void bulletToGLM(const btQuaternion& b, glm::quat& g) { 
    g.x = b.getX();
    g.y = b.getY();
    g.z = b.getZ();
    g.w = b.getW();
}

inline void glmToBullet(const glm::vec3& g, btVector3& b) {
    b.setX(g.x);
    b.setY(g.y);
    b.setZ(g.z);
}

inline void glmToBullet(const glm::quat& g, btQuaternion& b) {
    b = btQuaternion(g.x, g.y, g.z, g.w);
}

#endif // USE_BULLET_PHYSICS
#endif // hifi_BulletUtil_h
