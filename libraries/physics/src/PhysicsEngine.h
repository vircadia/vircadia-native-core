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

#include <stdint.h>

#include <QSet>
#include <btBulletDynamicsCommon.h>

#include <EntityItem.h>
#include <EntitySimulation.h>

#include "BulletUtil.h"
#include "ContactInfo.h"
#include "EntityMotionState.h"
#include "ShapeManager.h"
#include "ThreadSafeDynamicsWorld.h"

const float HALF_SIMULATION_EXTENT = 512.0f; // meters

class ObjectMotionState;

// simple class for keeping track of contacts
class ContactKey {
public:
    ContactKey() = delete;
    ContactKey(void* a, void* b) : _a(a), _b(b) {}
    bool operator<(const ContactKey& other) const { return _a < other._a || (_a == other._a && _b < other._b); }
    bool operator==(const ContactKey& other) const { return _a == other._a && _b == other._b; }
    void* _a;
    void* _b;
};

typedef std::map<ContactKey, ContactInfo> ContactMap;
typedef std::pair<ContactKey, ContactInfo> ContactMapElement;

class PhysicsEngine : public EntitySimulation {
public:
    // TODO: find a good way to make this a non-static method
    static uint32_t getNumSubsteps();

    PhysicsEngine() = delete; // prevent compiler from creating default ctor
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
    void stepNonPhysicalKinematics(const quint64& now);

    void computeCollisionEvents();

    /// \param offset position of simulation origin in domain-frame
    void setOriginOffset(const glm::vec3& offset) { _originOffset = offset; }

    /// \return position of simulation origin in domain-frame
    const glm::vec3& getOriginOffset() const { return _originOffset; }

    /// \param motionState pointer to Object's MotionState
    /// \return true if Object added
    void addObject(const ShapeInfo& shapeInfo, btCollisionShape* shape, ObjectMotionState* motionState);

    /// \param motionState pointer to Object's MotionState
    void removeObject(ObjectMotionState* motionState);

    /// process queue of changed from external sources
    void relayIncomingChangesToSimulation();

private:
    void removeContacts(ObjectMotionState* motionState);

    // return 'true' of update was successful
    bool updateObjectHard(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags);
    void updateObjectEasy(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags);

    btClock _clock;
    btDefaultCollisionConfiguration* _collisionConfig = NULL;
    btCollisionDispatcher* _collisionDispatcher = NULL;
    btBroadphaseInterface* _broadphaseFilter = NULL;
    btSequentialImpulseConstraintSolver* _constraintSolver = NULL;
    ThreadSafeDynamicsWorld* _dynamicsWorld = NULL;
    ShapeManager _shapeManager;

    glm::vec3 _originOffset;

    // EntitySimulation stuff
    QSet<EntityMotionState*> _entityMotionStates; // all entities that we track
    QSet<ObjectMotionState*> _nonPhysicalKinematicObjects; // not in physics simulation, but still need kinematic simulation
    QSet<ObjectMotionState*> _incomingChanges; // entities with pending physics changes by script or packet
    QSet<ObjectMotionState*> _outgoingPackets; // MotionStates with pending changes that need to be sent over wire

    EntityEditPacketSender* _entityPacketSender = NULL;

    ContactMap _contactMap;
    uint32_t _numContactFrames = 0;
    uint32_t _lastNumSubstepsAtUpdateInternal = 0;
};

#endif // hifi_PhysicsEngine_h
