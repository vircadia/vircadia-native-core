//
//  PhysicsWorld.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsWorld_h
#define hifi_PhysicsWorld_h

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>

#include "BulletUtil.h"
#include "CustomMotionState.h"
#include "PositionHashKey.h"
#include "ShapeManager.h"
#include "UUIDHashKey.h"
#include "VoxelObject.h"

class PhysicsWorld {
public:

    PhysicsWorld() : _collisionConfig(NULL), _collisionDispatcher(NULL), 
        _broadphaseFilter(NULL), _constraintSolver(NULL), _dynamicsWorld(NULL) { }

    ~PhysicsWorld();

    void init();

    /// \return true if Voxel added
    /// \param position the minimum corner of the voxel
    /// \param scale the length of the voxel side
    bool addVoxel(const glm::vec3& position, float scale);

    /// \return true if Voxel removed
    /// \param position the minimum corner of the voxel
    /// \param scale the length of the voxel side
    bool removeVoxel(const glm::vec3& position, float scale);

    /// \return true if Entity added
    /// \param info information about collision shapes to create
    bool addEntity(const QUuid& id, CustomMotionState* motionState);

    /// \return true if Entity removed
    /// \param id UUID of the entity
    bool removeEntity(const QUuid& id);

protected:
    btDefaultCollisionConfiguration* _collisionConfig;
    btCollisionDispatcher* _collisionDispatcher;
    btBroadphaseInterface* _broadphaseFilter;
    btSequentialImpulseConstraintSolver* _constraintSolver;
    btDiscreteDynamicsWorld* _dynamicsWorld;
    
    ShapeManager _shapeManager;

private:
    btHashMap<PositionHashKey, VoxelObject> _voxels;
    btHashMap<UUIDHashKey, CustomMotionState*> _entities;
};


#endif // USE_BULLET_PHYSICS
#endif // hifi_PhysicsWorld_h
