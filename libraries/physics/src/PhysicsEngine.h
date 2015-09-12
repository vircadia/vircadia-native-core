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

#include <QUuid>
#include <QVector>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "BulletUtil.h"
#include "ContactInfo.h"
#include "DynamicCharacterController.h"
#include "ObjectMotionState.h"
#include "ThreadSafeDynamicsWorld.h"
#include "ObjectAction.h"

const float HALF_SIMULATION_EXTENT = 512.0f; // meters

// simple class for keeping track of contacts
class ContactKey {
public:
    ContactKey() = delete;
    ContactKey(void* a, void* b) : _a(a), _b(b) {}
    bool operator<(const ContactKey& other) const { return _a < other._a || (_a == other._a && _b < other._b); }
    bool operator==(const ContactKey& other) const { return _a == other._a && _b == other._b; }
    void* _a; // ObjectMotionState pointer
    void* _b; // ObjectMotionState pointer
};

typedef std::map<ContactKey, ContactInfo> ContactMap;
typedef QVector<Collision> CollisionEvents;

class PhysicsEngine {
public:
    uint32_t getNumSubsteps();

    PhysicsEngine(const glm::vec3& offset);
    ~PhysicsEngine();
    void init();

    void setSessionUUID(const QUuid& sessionID) { _sessionID = sessionID; }
    const QUuid& getSessionID() const { return _sessionID; }

    void addObject(ObjectMotionState* motionState);
    void removeObject(ObjectMotionState* motionState);

    void deleteObjects(const VectorOfMotionStates& objects);
    void deleteObjects(const SetOfMotionStates& objects); // only called during teardown
    void addObjects(const VectorOfMotionStates& objects);
    VectorOfMotionStates changeObjects(const VectorOfMotionStates& objects);
    void reinsertObject(ObjectMotionState* object);

    void stepSimulation();
    void updateContactMap();

    bool hasOutgoingChanges() const { return _hasOutgoingChanges; }

    /// \return reference to list of changed MotionStates.  The list is only valid until beginning of next simulation loop.
    const VectorOfMotionStates& getOutgoingChanges();

    /// \return reference to list of Collision events.  The list is only valid until beginning of next simulation loop.
    const CollisionEvents& getCollisionEvents();

    /// \brief prints timings for last frame if stats have been requested.
    void dumpStatsIfNecessary();

    /// \param offset position of simulation origin in domain-frame
    void setOriginOffset(const glm::vec3& offset) { _originOffset = offset; }

    /// \return position of simulation origin in domain-frame
    const glm::vec3& getOriginOffset() const { return _originOffset; }

    /// \brief call bump on any objects that touch the object corresponding to motionState
    void bump(ObjectMotionState* motionState);

    void removeRigidBody(btRigidBody* body);

    void setCharacterController(DynamicCharacterController* character);

    void dumpNextStats() { _dumpNextStats = true; }

    int16_t getCollisionMask(int16_t group) const;

    EntityActionPointer getActionByID(const QUuid& actionID) const;
    void addAction(EntityActionPointer action);
    void removeAction(const QUuid actionID);

private:
    void removeContacts(ObjectMotionState* motionState);

    void doOwnershipInfection(const btCollisionObject* objectA, const btCollisionObject* objectB);

    btClock _clock;
    btDefaultCollisionConfiguration* _collisionConfig = NULL;
    btCollisionDispatcher* _collisionDispatcher = NULL;
    btBroadphaseInterface* _broadphaseFilter = NULL;
    btSequentialImpulseConstraintSolver* _constraintSolver = NULL;
    ThreadSafeDynamicsWorld* _dynamicsWorld = NULL;
    btGhostPairCallback* _ghostPairCallback = NULL;

    glm::vec3 _originOffset;

    ContactMap _contactMap;
    uint32_t _numContactFrames = 0;
    uint32_t _lastNumSubstepsAtUpdateInternal = 0;

    /// character collisions
    DynamicCharacterController* _characterController = NULL;

    bool _dumpNextStats = false;
    bool _hasOutgoingChanges = false;

    QUuid _sessionID;
    CollisionEvents _collisionEvents;

    QHash<QUuid, EntityActionPointer> _objectActions;

    btHashMap<btHashInt, int16_t> _collisionMasks;

    uint32_t _numSubsteps;
};

typedef std::shared_ptr<PhysicsEngine> PhysicsEnginePointer;

#endif // hifi_PhysicsEngine_h
