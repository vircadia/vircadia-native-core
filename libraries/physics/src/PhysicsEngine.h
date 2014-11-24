//
//  PhysicsEngine.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsEngine_h
#define hifi_PhysicsEngine_h

enum PhysicsUpdateFlag {
    PHYSICS_UPDATE_POSITION = 0x0001,
    PHYSICS_UPDATE_VELOCITY = 0x0002,
    PHYSICS_UPDATE_GRAVITY = 0x0004,
    PHYSICS_UPDATE_MASS = 0x0008,
    PHYSICS_UPDATE_COLLISION_GROUP = 0x0010,
    PHYSICS_UPDATE_MOTION_TYPE = 0x0020,
    PHYSICS_UPDATE_SHAPE = 0x0040
};

typedef unsigned int uint32_t;

// The update flags trigger two varieties of updates: "hard" which require the body to be pulled 
// and re-added to the physics engine and "easy" which just updates the body properties.
const uint32_t PHYSICS_UPDATE_HARD = PHYSICS_UPDATE_MOTION_TYPE | PHYSICS_UPDATE_SHAPE;
const uint32_t PHYSICS_UPDATE_EASY = PHYSICS_UPDATE_POSITION | PHYSICS_UPDATE_VELOCITY | 
        PHYSICS_UPDATE_GRAVITY | PHYSICS_UPDATE_MASS | PHYSICS_UPDATE_COLLISION_GROUP;

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>

#include "BulletUtil.h"
#include "CustomMotionState.h"
#include "PositionHashKey.h"
#include "ShapeManager.h"
#include "VoxelObject.h"

const float HALF_SIMULATION_EXTENT = 512.0f; // meters

class PhysicsEngine {
public:

    PhysicsEngine(const glm::vec3& offset)
        :   _collisionConfig(NULL), 
            _collisionDispatcher(NULL), 
            _broadphaseFilter(NULL), 
            _constraintSolver(NULL), 
            _dynamicsWorld(NULL),
            _originOffset(offset),
            _voxels() {
    }

    ~PhysicsEngine();

    virtual void init();

    void stepSimulation();

    /// \param offset position of simulation origin in domain-frame
    void setOriginOffset(const glm::vec3& offset) { _originOffset = offset; }

    /// \return position of simulation origin in domain-frame
    const glm::vec3& getOriginOffset() const { return _originOffset; }

    /// \param position the minimum corner of the voxel
    /// \param scale the length of the voxel side
    /// \return true if Voxel added
    bool addVoxel(const glm::vec3& position, float scale);

    /// \param position the minimum corner of the voxel
    /// \param scale the length of the voxel side
    /// \return true if Voxel removed
    bool removeVoxel(const glm::vec3& position, float scale);

    /// \param motionState pointer to Entity's MotionState
    /// \return true if Entity added
    bool addEntity(CustomMotionState* motionState);

    /// \param motionState pointer to Entity's MotionState
    /// \return true if Entity removed
    bool removeEntity(CustomMotionState* motionState);

    /// \param motionState pointer to Entity's MotionState
    /// \param flags set of bits indicating what categories of properties need to be updated
    /// \return true if entity updated
    bool updateEntity(CustomMotionState* motionState, uint32_t flags);

protected:
    void updateEntityHard(btRigidBody* body, CustomMotionState* motionState, uint32_t flags);
    void updateEntityEasy(btRigidBody* body, CustomMotionState* motionState, uint32_t flags);

    btClock _clock;
    btDefaultCollisionConfiguration* _collisionConfig;
    btCollisionDispatcher* _collisionDispatcher;
    btBroadphaseInterface* _broadphaseFilter;
    btSequentialImpulseConstraintSolver* _constraintSolver;
    btDiscreteDynamicsWorld* _dynamicsWorld;
    ShapeManager _shapeManager;

private:
    glm::vec3 _originOffset;
    btHashMap<PositionHashKey, VoxelObject> _voxels;
};

#else // USE_BULLET_PHYSICS
// PhysicsEngine stubbery until Bullet is required
class PhysicsEngine {
};
#endif // USE_BULLET_PHYSICS
#endif // hifi_PhysicsEngine_h
