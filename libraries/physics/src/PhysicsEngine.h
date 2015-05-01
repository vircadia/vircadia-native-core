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
#include "PhysicsTypedefs.h"
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
    void* _a; // ObjectMotionState pointer
    void* _b; // ObjectMotionState pointer
};

typedef std::map<ContactKey, ContactInfo> ContactMap;
typedef std::pair<ContactKey, ContactInfo> ContactMapElement;

class PhysicsEngine {
public:
    // TODO: find a good way to make this a non-static method
    static uint32_t getNumSubsteps();

    PhysicsEngine(const glm::vec3& offset);
    ~PhysicsEngine();
    void init();

    void setSessionUUID(const QUuid& sessionID) { _sessionID = sessionID; }

    void addObject(ObjectMotionState* motionState);
    void removeObject(ObjectMotionState* motionState);

    void deleteObjects(VectorOfMotionStates& objects);
    void deleteObjects(SetOfMotionStates& objects); // only called during teardown
    void addObjects(VectorOfMotionStates& objects);
    void changeObjects(VectorOfMotionStates& objects);
    void reinsertObject(ObjectMotionState* object);

    void stepSimulation();
    void computeCollisionEvents();
    void fireCollisionEvents();

    bool hasOutgoingChanges() const { return _hasOutgoingChanges; }
    VectorOfMotionStates& getOutgoingChanges();
    void dumpStatsIfNecessary();

    /// \param offset position of simulation origin in domain-frame
    void setOriginOffset(const glm::vec3& offset) { _originOffset = offset; }

    /// \return position of simulation origin in domain-frame
    const glm::vec3& getOriginOffset() const { return _originOffset; }

    void bump(ObjectMotionState* motionState);

    void removeRigidBody(btRigidBody* body);

    void setCharacterController(DynamicCharacterController* character);

    void dumpNextStats() { _dumpNextStats = true; }

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
};

#endif // hifi_PhysicsEngine_h
