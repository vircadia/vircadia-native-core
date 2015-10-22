//
//  CharacterControllerInterface.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CharacterControllerInterface_h
#define hifi_CharacterControllerInterface_h

#include <assert.h>
#include <stdint.h>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btCharacterControllerInterface.h>

const uint32_t PENDING_FLAG_ADD_TO_SIMULATION = 1U << 0;
const uint32_t PENDING_FLAG_REMOVE_FROM_SIMULATION = 1U << 1;
const uint32_t PENDING_FLAG_UPDATE_SHAPE = 1U << 2;
const uint32_t PENDING_FLAG_JUMP = 1U << 3;


class btRigidBody;
class btCollisionWorld;
class btDynamicsWorld;

class CharacterController : public btCharacterControllerInterface {
public:
    bool needsRemoval() const;
    bool needsAddition() const;
    void setDynamicsWorld(btDynamicsWorld* world);
    btCollisionObject* getCollisionObject() { return _rigidBody; }

    virtual void updateShapeIfNecessary() = 0;
    virtual void preSimulation() = 0;
    virtual void incrementSimulationTime(btScalar stepTime) = 0;
    virtual void postSimulation() = 0;

    virtual void setWalkDirection(const btVector3 &walkDirection) { assert(false); }

    /* these from btCharacterControllerInterface remain to be overridden
    virtual void setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval) = 0;
    virtual void reset() = 0;
    virtual void warp(const btVector3 &origin) = 0;
    virtual void preStep(btCollisionWorld *collisionWorld) = 0;
    virtual void playerStep(btCollisionWorld *collisionWorld, btScalar dt) = 0;
    virtual bool canJump() const = 0;
    virtual void jump() = 0;
    virtual bool onGround() const = 0;
    */
protected:
    btDynamicsWorld* _dynamicsWorld { nullptr };
    btRigidBody* _rigidBody { nullptr };
    uint32_t _pendingFlags { 0 };
};

#endif // hifi_CharacterControllerInterface_h
