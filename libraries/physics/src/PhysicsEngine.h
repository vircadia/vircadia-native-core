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

#include <QSet>
#include <btBulletDynamicsCommon.h>

#include <EntityItem.h>
#include <EntitySimulation.h>

#include "BulletUtil.h"
#include "CustomMotionState.h"
#include "PositionHashKey.h"
#include "ShapeManager.h"
#include "ThreadSafeDynamicsWorld.h"
#include "VoxelObject.h"

const float HALF_SIMULATION_EXTENT = 512.0f; // meters

class PhysicsEngine : public EntitySimulation {
public:

    PhysicsEngine(const glm::vec3& offset);

    ~PhysicsEngine();

    // override from EntitySimulation
    /// \param tree pointer to EntityTree which is stored internally
    void setEntityTree(EntityTree* tree);

    // override from EntitySimulation
    /// \param[out] entitiesToDelete list of entities removed from simulation and should be deleted.
    void updateEntities(QSet<EntityItem*>& entitiesToDelete);

    // override from EntitySimulation
    /// \param entity pointer to EntityItem to add to the simulation
    /// \sideeffect the EntityItem::_simulationState member may be updated to indicate membership to internal list
    void addEntity(EntityItem* entity);

    // override from EntitySimulation
    /// \param entity pointer to EntityItem to removed from the simulation
    /// \sideeffect the EntityItem::_simulationState member may be updated to indicate non-membership to internal list
    void removeEntity(EntityItem* entity);

    // override from EntitySimulation
    /// \param entity pointer to EntityItem to that may have changed in a way that would affect its simulation
    void entityChanged(EntityItem* entity);

    // override from EntitySimulation
    void clearEntities();

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
    bool addObject(CustomMotionState* motionState);

    /// \param motionState pointer to Object's MotionState
    /// \return true if Object removed
    bool removeObject(CustomMotionState* motionState);

    /// \param motionState pointer to Object's MotionState
    /// \param flags set of bits indicating what categories of properties need to be updated
    /// \return true if entity updated
    bool updateObject(CustomMotionState* motionState, uint32_t flags);

protected:
    void updateObjectHard(btRigidBody* body, CustomMotionState* motionState, uint32_t flags);
    void updateObjectEasy(btRigidBody* body, CustomMotionState* motionState, uint32_t flags);

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
    QSet<EntityItem*> _changedEntities;
    QSet<EntityItem*> _mortalEntities;
};

#else // USE_BULLET_PHYSICS
// PhysicsEngine stubbery until Bullet is required
class PhysicsEngine {
};
#endif // USE_BULLET_PHYSICS
#endif // hifi_PhysicsEngine_h
