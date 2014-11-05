//
//  VoxelObject.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelObject_h
#define hifi_VoxelObject_h

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

// VoxelObject is a simple wrapper for tracking a Voxel in a PhysicsWorld
class VoxelObject {
public:
    VoxelObject(const glm::vec3& center, btCollisionObject* object) : _object(object), _center(center) {
        assert(object != NULL);
    }
    btCollisionObject* _object;
    glm::vec3 _center;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_VoxelObject_h
