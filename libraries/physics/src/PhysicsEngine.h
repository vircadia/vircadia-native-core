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

/* Note: These are the collision groups defined in btBroadphaseProxy. Only 
 * DefaultFilter and StaticFilter are explicitly used by Bullet (when the 
 * collision filter of an object is not manually specified), the rest are 
 * merely suggestions.
 *
enum CollisionFilterGroups {
    DefaultFilter = 1,
    StaticFilter = 2,
    KinematicFilter = 4,
    DebrisFilter = 8,
    SensorTrigger = 16,
    CharacterFilter = 32,
    AllFilter = -1
}
 *
 * When using custom collision filters we pretty much need to do all or nothing. 
 * We'll be doing it all which means we define our own groups and build custom masks 
 * for everything.
 *
*/

const int16_t COLLISION_GROUP_DEFAULT = 1 << 0;
const int16_t COLLISION_GROUP_STATIC = 1 << 1;
const int16_t COLLISION_GROUP_KINEMATIC = 1 << 2;
const int16_t COLLISION_GROUP_DEBRIS = 1 << 3;
const int16_t COLLISION_GROUP_TRIGGER = 1 << 4;
const int16_t COLLISION_GROUP_MY_AVATAR = 1 << 5;
const int16_t COLLISION_GROUP_OTHER_AVATAR = 1 << 6;
const int16_t COLLISION_GROUP_MY_ATTACHMENT = 1 << 7;
const int16_t COLLISION_GROUP_OTHER_ATTACHMENT = 1 << 8;
// ...
const int16_t COLLISION_GROUP_COLLISIONLESS = 1 << 15;


/* Note: In order for objectA to collide with objectB at the filter stage 
 * both (groupA & maskB) and (groupB & maskA) must be non-zero.
 */
// DEFAULT collides with everything except COLLISIONLESS
const int16_t COLLISION_MASK_DEFAULT = ~ COLLISION_GROUP_COLLISIONLESS;
const int16_t COLLISION_MASK_STATIC = COLLISION_MASK_DEFAULT;
const int16_t COLLISION_MASK_KINEMATIC = COLLISION_MASK_DEFAULT;
// DEBRIS also doesn't collide with: other DEBRIS, and TRIGGER
const int16_t COLLISION_MASK_DEBRIS = ~ (COLLISION_GROUP_COLLISIONLESS
        | COLLISION_GROUP_DEBRIS
        | COLLISION_GROUP_TRIGGER);
// TRIGGER also doesn't collide with: DEBRIS, TRIGGER, and STATIC (TRIGGER only detects moveable things that matter)
const int16_t COLLISION_MASK_TRIGGER = COLLISION_MASK_DEBRIS & ~(COLLISION_GROUP_STATIC);
// AVATAR also doesn't collide with: corresponding ATTACHMENT
const int16_t COLLISION_MASK_MY_AVATAR = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_MY_ATTACHMENT);
const int16_t COLLISION_MASK_MY_ATTACHMENT = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_MY_AVATAR);
const int16_t COLLISION_MASK_OTHER_AVATAR = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_OTHER_ATTACHMENT);
const int16_t COLLISION_MASK_OTHER_ATTACHMENT = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_OTHER_AVATAR);
// ...
const int16_t COLLISION_MASK_COLLISIONLESS = 0;

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
    // TODO: find a good way to make this a non-static method
    static uint32_t getNumSubsteps();

    PhysicsEngine(const glm::vec3& offset);
    ~PhysicsEngine();
    void init();

    void setSessionUUID(const QUuid& sessionID) { _sessionID = sessionID; }
    const QUuid& getSessionID() const { return _sessionID; }

    void addObject(ObjectMotionState* motionState);
    void removeObject(ObjectMotionState* motionState);

    void deleteObjects(VectorOfMotionStates& objects);
    void deleteObjects(SetOfMotionStates& objects); // only called during teardown
    void addObjects(VectorOfMotionStates& objects);
    void changeObjects(VectorOfMotionStates& objects);
    void reinsertObject(ObjectMotionState* object);

    void stepSimulation();
    void updateContactMap();

    bool hasOutgoingChanges() const { return _hasOutgoingChanges; }

    /// \return reference to list of changed MotionStates.  The list is only valid until beginning of next simulation loop.
    VectorOfMotionStates& getOutgoingChanges();

    /// \return reference to list of Collision events.  The list is only valid until beginning of next simulation loop.
    CollisionEvents& getCollisionEvents();

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

    // TODO: Andrew to move these to ObjectMotionState
    static bool physicsInfoIsActive(void* physicsInfo);
    static bool getBodyLocation(void* physicsInfo, glm::vec3& positionReturn, glm::quat& rotationReturn);

    int16_t getCollisionMask(int16_t group) const;

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
    btHashMap<btHashInt, int16_t> _collisionMasks;
};

#endif // hifi_PhysicsEngine_h
