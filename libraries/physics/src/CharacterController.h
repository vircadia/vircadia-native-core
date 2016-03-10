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

#include <GLMHelpers.h>
#include "BulletUtil.h"

const uint32_t PENDING_FLAG_ADD_TO_SIMULATION = 1U << 0;
const uint32_t PENDING_FLAG_REMOVE_FROM_SIMULATION = 1U << 1;
const uint32_t PENDING_FLAG_UPDATE_SHAPE = 1U << 2;
const uint32_t PENDING_FLAG_JUMP = 1U << 3;

const float DEFAULT_CHARACTER_GRAVITY = -5.0f;

class btRigidBody;
class btCollisionWorld;
class btDynamicsWorld;

//#define DEBUG_STATE_CHANGE

class CharacterController : public btCharacterControllerInterface {
public:
    CharacterController();
    virtual ~CharacterController() {}

    bool needsRemoval() const;
    bool needsAddition() const;
    void setDynamicsWorld(btDynamicsWorld* world);
    btCollisionObject* getCollisionObject() { return _rigidBody; }

    virtual void updateShapeIfNecessary() = 0;

    // overrides from btCharacterControllerInterface
    virtual void setWalkDirection(const btVector3 &walkDirection) override { assert(false); }
    virtual void setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval) override { assert(false); }
    virtual void reset(btCollisionWorld* collisionWorld) override { }
    virtual void warp(const btVector3& origin) override { }
    virtual void debugDraw(btIDebugDraw* debugDrawer) override { }
    virtual void setUpInterpolate(bool value) override { }
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) override {
        preStep(collisionWorld);
        playerStep(collisionWorld, deltaTime);
    }
    virtual void preStep(btCollisionWorld *collisionWorld) override;
    virtual void playerStep(btCollisionWorld *collisionWorld, btScalar dt) override;
    virtual bool canJump() const override { assert(false); return false; } // never call this
    virtual void jump() override;
    virtual bool onGround() const override;

    void preSimulation();
    void postSimulation();

    void setPositionAndOrientation( const glm::vec3& position, const glm::quat& orientation);
    void getPositionAndOrientation(glm::vec3& position, glm::quat& rotation) const;

    void setTargetVelocity(const glm::vec3& velocity);
    void setParentVelocity(const glm::vec3& parentVelocity);
    void setFollowParameters(const glm::mat4& desiredWorldMatrix, float timeRemaining);
    float getFollowTime() const { return _followTime; }
    glm::vec3 getFollowLinearDisplacement() const;
    glm::quat getFollowAngularDisplacement() const;
    glm::vec3 getFollowVelocity() const;

    glm::vec3 getLinearVelocity() const;

    float getCapsuleRadius() const { return _radius; }
    float getCapsuleHalfHeight() const { return _halfHeight; }
    glm::vec3 getCapsuleLocalOffset() const { return _shapeLocalOffset; }

    enum class State {
        Ground = 0,
        Takeoff,
        InAir,
        Hover
    };

    State getState() const { return _state; }

    void setLocalBoundingBox(const glm::vec3& corner, const glm::vec3& scale);

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled && _dynamicsWorld; }

    bool getRigidBodyLocation(glm::vec3& avatarRigidBodyPosition, glm::quat& avatarRigidBodyRotation);

protected:
#ifdef DEBUG_STATE_CHANGE
    void setState(State state, const char* reason);
#else
    void setState(State state);
#endif

    void updateUpAxis(const glm::quat& rotation);
    bool checkForSupport(btCollisionWorld* collisionWorld) const;

protected:
    btVector3 _currentUp;
    btVector3 _targetVelocity;
    btVector3 _parentVelocity;
    btTransform _followDesiredBodyTransform;
    btScalar _followTimeRemaining;
    btTransform _characterBodyTransform;

    glm::vec3 _shapeLocalOffset;

    glm::vec3 _boxScale; // used to compute capsule shape

    quint64 _rayHitStartTime;
    quint64 _takeoffToInAirStartTime;
    quint64 _jumpButtonDownStartTime;
    quint32 _jumpButtonDownCount;
    quint32 _takeoffJumpButtonID;

    btScalar _halfHeight;
    btScalar _radius;

    btScalar _floorDistance;
    bool _hasSupport;

    btScalar _gravity;

    btScalar _jumpSpeed;
    btScalar _followTime;
    btVector3 _followLinearDisplacement;
    btQuaternion _followAngularDisplacement;

    bool _enabled;
    State _state;
    bool _isPushingUp;

    btDynamicsWorld* _dynamicsWorld { nullptr };
    btRigidBody* _rigidBody { nullptr };
    uint32_t _pendingFlags { 0 };
    uint32_t _previousFlags { 0 };
};

#endif // hifi_CharacterControllerInterface_h
