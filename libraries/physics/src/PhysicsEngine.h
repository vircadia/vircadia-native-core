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
    /// \return true if entity updated
    bool updateEntityMotionType(CustomMotionState* motionState);

    bool updateEntityMassProperties(CustomMotionState* motionState, float mass, const glm::vec3& inertiaEigenValues);

protected:
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
