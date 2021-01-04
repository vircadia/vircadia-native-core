//
//  CharacterControllerInterface.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CharacterController_h
#define hifi_CharacterController_h

#include <assert.h>
#include <stdint.h>
#include <atomic>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btCharacterControllerInterface.h>

#include <GLMHelpers.h>
#include <NumericalConstants.h>

#include "AvatarConstants.h"
#include "BulletUtil.h"
#include "CharacterGhostObject.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"

const uint32_t PENDING_FLAG_ADD_TO_SIMULATION = 1U << 0;
const uint32_t PENDING_FLAG_REMOVE_FROM_SIMULATION = 1U << 1;
const uint32_t PENDING_FLAG_UPDATE_SHAPE = 1U << 2;
const uint32_t PENDING_FLAG_JUMP = 1U << 3;
const uint32_t PENDING_FLAG_UPDATE_COLLISION_MASK = 1U << 4;
const uint32_t PENDING_FLAG_RECOMPUTE_FLYING = 1U << 5;
const uint32_t PENDING_FLAG_ADD_DETAILED_TO_SIMULATION = 1U << 6;
const uint32_t PENDING_FLAG_REMOVE_DETAILED_FROM_SIMULATION = 1U << 7;

const float DEFAULT_MIN_FLOOR_NORMAL_DOT_UP = cosf(PI / 3.0f);

const uint32_t NUM_SUBSTEPS_FOR_STUCK_TRANSITION = 6; // physics substeps
const uint32_t NUM_SUBSTEPS_FOR_SAFE_LANDING_RETRY = NUM_SUBSTEPS_PER_SECOND / 2; // retry every half second

class btRigidBody;
class btCollisionWorld;
class btDynamicsWorld;

//#define DEBUG_STATE_CHANGE

const btScalar MAX_CHARACTER_MOTOR_TIMESCALE = 60.0f; // one minute
const btScalar MIN_CHARACTER_MOTOR_TIMESCALE = 0.05f;

class CharacterController : public btCharacterControllerInterface {

public:
    enum class FollowType : uint8_t {
        Rotation,
        Horizontal,
        Vertical,
        Count
    };

    // Remaining follow time for each FollowType
    typedef std::array<float, static_cast<size_t>(FollowType::Count)> FollowTimePerType;

    // Follow time value meaning that we should snap immediately to the target.
    static constexpr float FOLLOW_TIME_IMMEDIATE_SNAP = FLT_MAX;

    CharacterController(const FollowTimePerType& followTimeRemainingPerType);
    virtual ~CharacterController();
    bool needsRemoval() const;
    bool needsAddition() const;
    virtual void addToWorld();
    void removeFromWorld();
    btCollisionObject* getCollisionObject() { return _rigidBody; }

    void setGravity(float gravity);
    float getGravity();
    void recomputeFlying();

    virtual void updateShapeIfNecessary() = 0;

    // overrides from btCharacterControllerInterface
    virtual void setWalkDirection(const btVector3 &walkDirection) override { assert(false); }
    virtual void setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval) override { assert(false); }
    virtual void reset(btCollisionWorld* collisionWorld) override {}
    virtual void warp(const btVector3& origin) override {}
    virtual void debugDraw(btIDebugDraw* debugDrawer) override {}
    virtual void setUpInterpolate(bool value) override {}
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) override;
    virtual void preStep(btCollisionWorld *collisionWorld) override;
    virtual void playerStep(btCollisionWorld *collisionWorld, btScalar dt) override;
    virtual bool canJump() const override { assert(false); return false; } // never call this
    virtual void jump(const btVector3& dir = btVector3(0.0f, 0.0f, 0.0f)) override;
    virtual bool onGround() const override;

    void clearMotors();
    void addMotor(const glm::vec3& velocity, const glm::quat& rotation, float horizTimescale, float vertTimescale = -1.0f);
    void applyMotor(int index, btScalar dt, btVector3& worldVelocity, std::vector<btVector3>& velocities, std::vector<btScalar>& weights);
    void setStepUpEnabled(bool enabled) { _stepUpEnabled = enabled; }
    void computeNewVelocity(btScalar dt, btVector3& velocity);
    void computeNewVelocity(btScalar dt, glm::vec3& velocity);
    void setScaleFactor(btScalar scaleFactor) { _scaleFactor = scaleFactor; }

    // HACK for legacy 'thrust' feature
    void setLinearAcceleration(const glm::vec3& acceleration) { _linearAcceleration = glmToBullet(acceleration); }

    void preSimulation();
    void postSimulation();

    void setPositionAndOrientation(const glm::vec3& position, const glm::quat& orientation);
    void getPositionAndOrientation(glm::vec3& position, glm::quat& rotation) const;

    void setParentVelocity(const glm::vec3& parentVelocity);

    void setFollowParameters(const glm::mat4& desiredWorldMatrix);
    float getFollowTime() const { return _followTime; }
    glm::vec3 getFollowLinearDisplacement() const;
    glm::quat getFollowAngularDisplacement() const;
    glm::vec3 getFollowVelocity() const;

    glm::vec3 getLinearVelocity() const;
    glm::vec3 getVelocityChange() const;

    float getCapsuleRadius() const { return _radius; }
    float getCapsuleHalfHeight() const { return _halfHeight; }
    glm::vec3 getCapsuleLocalOffset() const { return _shapeLocalOffset; }

    enum class State {
        Ground = 0,
        Takeoff,
        InAir,
        Hover,
        Seated
    };

    State getState() const { return _state; }
    void updateState();

    void setLocalBoundingBox(const glm::vec3& minCorner, const glm::vec3& scale);

    void setPhysicsEngine(const PhysicsEnginePointer& engine);
    bool isEnabledAndReady() const { return (bool)_physicsEngine; }
    bool isStuck() const { return _isStuck; }
    float getCollisionBrakeAttenuationFactor() const;

    void setCollisionless(bool collisionless);

    virtual int32_t computeCollisionMask() const = 0;
    virtual void handleChangedCollisionMask() = 0;

    bool getRigidBodyLocation(glm::vec3& avatarRigidBodyPosition, glm::quat& avatarRigidBodyRotation);

    void setZoneFlyingAllowed(bool value) { _zoneFlyingAllowed = value; }
    void setComfortFlyingAllowed(bool value) { _comfortFlyingAllowed = value; }
    void setHoverWhenUnsupported(bool value) { _hoverWhenUnsupported = value; }
    void setCollisionlessAllowed(bool value);

    void setPendingFlagsUpdateCollisionMask(){ _pendingFlags |= PENDING_FLAG_UPDATE_COLLISION_MASK; }
    void setSeated(bool isSeated) { _isSeated = isSeated;  }
    bool getSeated() const { return _isSeated; }

    void resetStuckCounter() { _numStuckSubsteps = 0; }

protected:
#ifdef DEBUG_STATE_CHANGE
    void setState(State state, const char* reason);
#else
    void setState(State state);
#endif

    virtual void updateMassProperties() = 0;
    void updateCurrentGravity();
    void updateUpAxis(const glm::quat& rotation);
    bool checkForSupport(btCollisionWorld* collisionWorld);

protected:
    struct CharacterMotor {
        CharacterMotor(const glm::vec3& vel, const glm::quat& rot, float horizTimescale, float vertTimescale = -1.0f);

        btVector3 velocity { btVector3(0.0f, 0.0f, 0.0f) }; // local-frame
        btQuaternion rotation; // local-to-world
        btScalar hTimescale { MAX_CHARACTER_MOTOR_TIMESCALE }; // horizontal
        btScalar vTimescale { MAX_CHARACTER_MOTOR_TIMESCALE }; // vertical
    };

    std::vector<CharacterMotor> _motors;
    CharacterGhostObject _ghost;
    btVector3 _currentUp;
    btVector3 _targetVelocity;
    btVector3 _parentVelocity;
    btVector3 _preSimulationVelocity;
    btVector3 _velocityChange;
    btTransform _followDesiredBodyTransform;
    const FollowTimePerType& _followTimeRemainingPerType;
    btTransform _characterBodyTransform;
    btVector3 _position;
    btQuaternion _rotation;

    glm::vec3 _shapeLocalOffset;

    glm::vec3 _boxScale; // used to compute capsule shape

    quint64 _rayHitStartTime;
    quint64 _takeoffToInAirStartTime;
    quint64 _jumpButtonDownStartTime;
    quint32 _jumpButtonDownCount;
    quint32 _takeoffJumpButtonID;

    // data for walking up steps
    btVector3 _stepPoint { 0.0f, 0.0f, 0.0f };
    btVector3 _stepNormal { 0.0f, 0.0f, 0.0f };
    btScalar _stepHeight { 0.0f };
    btScalar _minStepHeight { 0.0f };
    btScalar _maxStepHeight { 0.0f };
    btScalar _minFloorNormalDotUp { DEFAULT_MIN_FLOOR_NORMAL_DOT_UP };

    btScalar _halfHeight { 0.0f };
    btScalar _radius { 0.0f };

    btScalar _floorDistance;
    bool _steppingUp { false };
    bool _stepUpEnabled { true };
    bool _hasSupport;

    btScalar _currentGravity { 0.0f };
    btScalar _gravity { DEFAULT_AVATAR_GRAVITY };

    btScalar _followTime;
    btVector3 _followLinearDisplacement;
    btQuaternion _followAngularDisplacement;
    btVector3 _linearAcceleration;
    btVector3 _netCollisionImpulse;

    State _state;
    bool _isPushingUp;
    bool _isStuck { false };
    bool _isSeated { false };
    float _collisionBrake { 0.0f };

    PhysicsEnginePointer _physicsEngine { nullptr };
    btRigidBody* _rigidBody { nullptr };
    uint32_t _pendingFlags { 0 };
    uint32_t _previousFlags { 0 };
    uint32_t _stuckTransitionCount { 0 };
    uint32_t _numStuckSubsteps { 0 };

    bool _inWorld { false };
    bool _zoneFlyingAllowed { true };
    bool _comfortFlyingAllowed { true };
    bool _hoverWhenUnsupported{ true };
    bool _collisionlessAllowed { true };
    bool _collisionless { false };

    btScalar _scaleFactor { 1.0f };
};

#endif // hifi_CharacterController_h
