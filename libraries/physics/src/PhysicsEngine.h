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

#include <QSet>
#include <btBulletDynamicsCommon.h>

#include <EntityItem.h>
#include <EntitySimulation.h>

#include "BulletUtil.h"
#include "EntityMotionState.h"
#include "ShapeManager.h"
#include "ThreadSafeDynamicsWorld.h"

const float HALF_SIMULATION_EXTENT = 512.0f; // meters

class PhysicsEngine : public EntitySimulation {
public:

    PhysicsEngine(const glm::vec3& offset);

    ~PhysicsEngine();

    // overrides for EntitySimulation
    void updateEntitiesInternal(const quint64& now);
    void addEntityInternal(EntityItem* entity);
    void removeEntityInternal(EntityItem* entity);
    void entityChangedInternal(EntityItem* entity);
    void sortEntitiesThatMovedInternal();
    void clearEntitiesInternal();

    virtual void init(EntityEditPacketSender* packetSender);

    void stepSimulation();

    /// \param offset position of simulation origin in domain-frame
    void setOriginOffset(const glm::vec3& offset) { _originOffset = offset; }

    /// \return position of simulation origin in domain-frame
    const glm::vec3& getOriginOffset() const { return _originOffset; }

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

    // EntitySimulation stuff
    QSet<EntityMotionState*> _entityMotionStates; // all entities that we track
    QSet<ObjectMotionState*> _incomingChanges; // entities with pending physics changes by script or packet
    QSet<ObjectMotionState*> _outgoingPackets; // MotionStates with pending changes that need to be sent over wire

    EntityEditPacketSender* _entityPacketSender;

    uint32_t _frameCount;
    btScalar _infinity = BT_INFINITY; // HACK: eliminates unused variable warning from Bullet headers
};

#endif // hifi_PhysicsEngine_h
