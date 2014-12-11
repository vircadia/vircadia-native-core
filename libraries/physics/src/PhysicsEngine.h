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

typedef unsigned int uint32_t;

#ifdef USE_BULLET_PHYSICS

#include <QSet>
#include <btBulletDynamicsCommon.h>

#include <EntityItem.h>
#include <EntitySimulation.h>

#include "BulletUtil.h"
#include "EntityMotionState.h"
#include "PositionHashKey.h"
#include "ShapeManager.h"
#include "ThreadSafeDynamicsWorld.h"
#include "VoxelObject.h"

const float HALF_SIMULATION_EXTENT = 512.0f; // meters

class PhysicsEngine : public EntitySimulation {
public:

    PhysicsEngine(const glm::vec3& offset);

    ~PhysicsEngine();

    // overrides from EntitySimulation
    void updateEntitiesInternal(const quint64& now);
    void addEntityInternal(EntityItem* entity);
    void removeEntityInternal(EntityItem* entity);
    void entityChangedInternal(EntityItem* entity);
    void clearEntitiesInternal();

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

    /// \param motionState pointer to Object's MotionState
    /// \return true if Object added
    bool addObject(ObjectMotionState* motionState);

    /// \param motionState pointer to Object's MotionState
    /// \return true if Object removed
    bool removeObject(ObjectMotionState* motionState);

    /// process queue of changed from external sources
    void relayIncomingChangesToSimulation();

    /// \return duration of fixed simulation substep
    float getFixedSubStep() const;

    /// \return number of simulation frames the physics engine has taken
    uint32_t getFrameCount() const { return _frameCount; }

    /// \return substep remainder used for Bullet MotionState extrapolation
    // Bullet will extrapolate the positions provided to MotionState::setWorldTransform() in an effort to provide 
    // smoother visible motion when the render frame rate does not match that of the simulation loop.  We provide 
    // access to this fraction for improved filtering of update packets to interested parties.
    float getSubStepRemainder() { return _dynamicsWorld->getLocalTimeAccumulation(); }

protected:
    void updateObjectHard(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags);
    void updateObjectEasy(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags);

    btClock _clock;
    btDefaultCollisionConfiguration* _collisionConfig;
    btCollisionDispatcher* _collisionDispatcher;
    btBroadphaseInterface* _broadphaseFilter;
    btSequentialImpulseConstraintSolver* _constraintSolver;
    ThreadSafeDynamicsWorld* _dynamicsWorld;
    ShapeManager _shapeManager;

private:
    glm::vec3 _originOffset;
    btHashMap<PositionHashKey, VoxelObject> _voxels;

    // EntitySimulation stuff
    QSet<EntityMotionState*> _entityMotionStates; // all entities that we track
    QSet<ObjectMotionState*> _incomingChanges; // entities with pending physics changes by script or packet
    QSet<ObjectMotionState*> _outgoingPhysics; // MotionStates with pending transform changes from physics simulation

    EntityEditPacketSender* _entityPacketSender;

    uint32_t _frameCount;
};

#else // USE_BULLET_PHYSICS
// PhysicsEngine stubbery until Bullet is required
class PhysicsEngine {
};
#endif // USE_BULLET_PHYSICS
#endif // hifi_PhysicsEngine_h
