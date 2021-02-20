//
//  CharacterControllerInterface.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CharacterController.h"

#include <AvatarConstants.h>
#include <NumericalConstants.h>
#include <PhysicsCollisionGroups.h>

#include "ObjectMotionState.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"
#include "TemporaryPairwiseCollisionFilter.h"

const float STUCK_PENETRATION = -0.05f; // always negative into the object.
const float STUCK_IMPULSE = 500.0f;


const btVector3 LOCAL_UP_AXIS(0.0f, 1.0f, 0.0f);
static bool _appliedStuckRecoveryStrategy = false;

static TemporaryPairwiseCollisionFilter _pairwiseFilter;

// Note: applyPairwiseFilter is registered as a sub-callback to Bullet's gContactAddedCallback feature
// when we detect MyAvatar is "stuck".  It will disable new ManifoldPoints between MyAvatar and mesh objects with
// which it has deep penetration, and will continue disabling new contact until new contacts stop happening
// (no overlap).  If MyAvatar is not trying to move its velocity is defaulted to "up", to help it escape overlap.
bool applyPairwiseFilter(btManifoldPoint& cp,
        const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
        const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
    static int32_t numCalls = 0;
    ++numCalls;
    // This callback is ONLY called on objects with btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK flag
    // and the flagged object will always be sorted to Obj0.  Hence the "other" is always Obj1.
    const btCollisionObject* other = colObj1Wrap->m_collisionObject;

    if (_pairwiseFilter.isFiltered(other)) {
        _pairwiseFilter.incrementEntry(other);
        // disable contact point by setting distance too large and normal to zero
        cp.setDistance(1.0e6f);
        cp.m_normalWorldOnB.setValue(0.0f, 0.0f, 0.0f);
        _appliedStuckRecoveryStrategy = true;
        return false;
    }

    if (other->getCollisionShape()->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE) {
        if ( cp.getDistance() < 2.0f * STUCK_PENETRATION ||
                cp.getAppliedImpulse() > 2.0f * STUCK_IMPULSE ||
                (cp.getDistance() < STUCK_PENETRATION && cp.getAppliedImpulse() > STUCK_IMPULSE)) {
            _pairwiseFilter.incrementEntry(other);
            // disable contact point by setting distance too large and normal to zero
            cp.setDistance(1.0e6f);
            cp.m_normalWorldOnB.setValue(0.0f, 0.0f, 0.0f);
            _appliedStuckRecoveryStrategy = true;
        }
    }
    return false;
}

#ifdef DEBUG_STATE_CHANGE
#define SET_STATE(desiredState, reason) setState(desiredState, reason)
#else
#define SET_STATE(desiredState, reason) setState(desiredState)
#endif

// helper class for simple ray-traces from character
class ClosestNotMe : public btCollisionWorld::ClosestRayResultCallback {
public:
    ClosestNotMe(btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
        _me = me;
        // the RayResultCallback's group and mask must match MY_AVATAR
        m_collisionFilterGroup = BULLET_COLLISION_GROUP_MY_AVATAR;
        m_collisionFilterMask = BULLET_COLLISION_MASK_MY_AVATAR;
    }
    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) override {
        if (rayResult.m_collisionObject == _me) {
            return 1.0f;
        }
        return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
protected:
    btRigidBody* _me;
};

CharacterController::CharacterMotor::CharacterMotor(const glm::vec3& vel, const glm::quat& rot, float horizTimescale, float vertTimescale) {
    velocity = glmToBullet(vel);
    rotation = glmToBullet(rot);
    hTimescale = horizTimescale;
    if (hTimescale < MIN_CHARACTER_MOTOR_TIMESCALE) {
        hTimescale = MIN_CHARACTER_MOTOR_TIMESCALE;
    }
    vTimescale = vertTimescale;
    if (vTimescale < 0.0f) {
        vTimescale = hTimescale;
    } else if (vTimescale < MIN_CHARACTER_MOTOR_TIMESCALE) {
        vTimescale = MIN_CHARACTER_MOTOR_TIMESCALE;
    }
}

static uint32_t _numCharacterControllers { 0 };

CharacterController::CharacterController(const FollowTimePerType& followTimeRemainingPerType) :
    _followTimeRemainingPerType(followTimeRemainingPerType) {
    _floorDistance = _scaleFactor * DEFAULT_AVATAR_FALL_HEIGHT;

    _targetVelocity.setValue(0.0f, 0.0f, 0.0f);
    _followDesiredBodyTransform.setIdentity();
    _state = State::Hover;
    _isPushingUp = false;
    _rayHitStartTime = 0;
    _takeoffToInAirStartTime = 0;
    _jumpButtonDownStartTime = 0;
    _jumpButtonDownCount = 0;
    _followTime = 0.0f;
    _followLinearDisplacement = btVector3(0, 0, 0);
    _followAngularDisplacement = btQuaternion::getIdentity();
    _hasSupport = false;

    _pendingFlags = PENDING_FLAG_UPDATE_SHAPE;

    // ATM CharacterController is a singleton.  When we want more we'll have to
    // overhaul the applyPairwiseFilter() logic to handle multiple instances.
    ++_numCharacterControllers;
    assert(_numCharacterControllers == 1);
}

CharacterController::~CharacterController() {
    if (_rigidBody) {
        btCollisionShape* shape = _rigidBody->getCollisionShape();
        if (shape) {
            delete shape;
        }
        delete _rigidBody;
        _rigidBody = nullptr;
    }
}

bool CharacterController::needsRemoval() const {
    return (_physicsEngine && (_pendingFlags & PENDING_FLAG_REMOVE_FROM_SIMULATION) == PENDING_FLAG_REMOVE_FROM_SIMULATION);
}

bool CharacterController::needsAddition() const {
    return (_physicsEngine && (_pendingFlags & PENDING_FLAG_ADD_TO_SIMULATION) == PENDING_FLAG_ADD_TO_SIMULATION);
}

void CharacterController::removeFromWorld() {
    if (_inWorld) {
        if (_rigidBody) {
            _physicsEngine->getDynamicsWorld()->removeRigidBody(_rigidBody);
            _physicsEngine->getDynamicsWorld()->removeAction(this);
        }
        _inWorld = false;
    }
    _pendingFlags &= ~PENDING_FLAG_REMOVE_FROM_SIMULATION;
}

void CharacterController::addToWorld() {
    if (!_rigidBody) {
        return;
    }
    if (_inWorld) {
        _pendingFlags &= ~PENDING_FLAG_ADD_DETAILED_TO_SIMULATION;
        return;
    }
    btDiscreteDynamicsWorld* world = _physicsEngine->getDynamicsWorld();
    int32_t collisionMask = computeCollisionMask();
    int32_t collisionGroup = BULLET_COLLISION_GROUP_MY_AVATAR;

    updateMassProperties();
    _pendingFlags &= ~PENDING_FLAG_ADD_DETAILED_TO_SIMULATION;

    // add to new world
    _pendingFlags &= ~PENDING_FLAG_JUMP;
    world->addRigidBody(_rigidBody, collisionGroup, collisionMask);
    world->addAction(this);
    _inWorld = true;

    // restore gravity settings because adding an object to the world overwrites its gravity setting
    _rigidBody->setGravity(_currentGravity * _currentUp);
    // set flag to enable custom contactAddedCallback
    _rigidBody->setCollisionFlags(btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

    // enable CCD
    _rigidBody->setCcdSweptSphereRadius(_halfHeight);
    _rigidBody->setCcdMotionThreshold(_radius);

    btCollisionShape* shape = _rigidBody->getCollisionShape();
    assert(shape && shape->getShapeType() == CONVEX_HULL_SHAPE_PROXYTYPE);
    _ghost.setCharacterShape(static_cast<btConvexHullShape*>(shape));

    _ghost.setCollisionGroupAndMask(collisionGroup, collisionMask & (~ collisionGroup));
    _ghost.setCollisionWorld(world);
    _ghost.setRadiusAndHalfHeight(_radius, _halfHeight);
    if (_rigidBody) {
        _ghost.setWorldTransform(_rigidBody->getWorldTransform());
    }
    if (_pendingFlags & PENDING_FLAG_UPDATE_SHAPE) {
        // shouldn't fall in here, but if we do make sure both ADD and REMOVE bits are still set
        _pendingFlags |= PENDING_FLAG_ADD_TO_SIMULATION | PENDING_FLAG_REMOVE_FROM_SIMULATION |
                         PENDING_FLAG_ADD_DETAILED_TO_SIMULATION | PENDING_FLAG_REMOVE_DETAILED_FROM_SIMULATION;
    } else {
        _pendingFlags &= ~PENDING_FLAG_ADD_TO_SIMULATION;
    }
}

bool CharacterController::checkForSupport(btCollisionWorld* collisionWorld) {
    bool pushing = _targetVelocity.length2() > FLT_EPSILON;

    btDispatcher* dispatcher = collisionWorld->getDispatcher();
    int numManifolds = dispatcher->getNumManifolds();
    bool hasFloor = false;
    bool probablyStuck = _isStuck && _appliedStuckRecoveryStrategy;

    btTransform rotation = _rigidBody->getWorldTransform();
    rotation.setOrigin(btVector3(0.0f, 0.0f, 0.0f)); // clear translation part

    float deepestDistance = 0.0f;
    float strongestImpulse = 0.0f;

    _netCollisionImpulse = btVector3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
        if (_rigidBody == contactManifold->getBody1() || _rigidBody == contactManifold->getBody0()) {
            bool characterIsFirst = _rigidBody == contactManifold->getBody0();
            int numContacts = contactManifold->getNumContacts();
            int stepContactIndex = -1;
            bool stepValid = true;
            float highestStep = _minStepHeight;
            for (int j = 0; j < numContacts; j++) {
                // check for "floor"
                btManifoldPoint& contact = contactManifold->getContactPoint(j);
                btVector3 pointOnCharacter = characterIsFirst ? contact.m_localPointA : contact.m_localPointB; // object-local-frame
                btVector3 normal = characterIsFirst ? contact.m_normalWorldOnB : -contact.m_normalWorldOnB; // points toward character
                btScalar hitHeight = _halfHeight + _radius + pointOnCharacter.dot(_currentUp);

                float distance = contact.getDistance();
                if (distance < deepestDistance) {
                    deepestDistance = distance;
                }
                float impulse = contact.getAppliedImpulse();
                _netCollisionImpulse += impulse * normal;
                if (impulse > strongestImpulse) {
                    strongestImpulse = impulse;
                }

                if (hitHeight < _maxStepHeight && normal.dot(_currentUp) > _minFloorNormalDotUp) {
                    hasFloor = true;
                }
                if (stepValid && pushing && _targetVelocity.dot(normal) < 0.0f) {
                    // remember highest step obstacle
                    if (!_stepUpEnabled || hitHeight > _maxStepHeight) {
                        // this manifold is invalidated by point that is too high
                        stepValid = false;
                    } else if (hitHeight > highestStep && normal.dot(_targetVelocity) < 0.0f ) {
                        highestStep = hitHeight;
                        stepContactIndex = j;
                        hasFloor = true;
                    }
                }
            }
            if (stepValid && stepContactIndex > -1 && highestStep > _stepHeight) {
                // remember step info for later
                btManifoldPoint& contact = contactManifold->getContactPoint(stepContactIndex);
                btVector3 pointOnCharacter = characterIsFirst ? contact.m_localPointA : contact.m_localPointB; // object-local-frame
                _stepNormal = characterIsFirst ? contact.m_normalWorldOnB : -contact.m_normalWorldOnB; // points toward character
                _stepHeight = highestStep;
                _stepPoint = rotation * pointOnCharacter; // rotate into world-frame
            }
        }
    }

    // If there's deep penetration and big impulse we're probably stuck.
    probablyStuck = probablyStuck
        || deepestDistance < 2.0f * STUCK_PENETRATION
        || strongestImpulse > 2.0f * STUCK_IMPULSE
        || (deepestDistance < STUCK_PENETRATION && strongestImpulse > STUCK_IMPULSE);

    if (_isStuck != probablyStuck) {
        ++_stuckTransitionCount;
        if (_stuckTransitionCount > NUM_SUBSTEPS_FOR_STUCK_TRANSITION) {
            // we've been in this "probablyStuck" state for several consecutive substeps
            // --> make it official by changing state
            qCDebug(physics) << "CharacterController::_isStuck :" << _isStuck << "-->" << probablyStuck;
            _isStuck = probablyStuck;
            // init _numStuckSubsteps at NUM_SUBSTEPS_FOR_SAFE_LANDING_RETRY so SafeLanding tries to help immediately
            _numStuckSubsteps = NUM_SUBSTEPS_FOR_SAFE_LANDING_RETRY;
            _stuckTransitionCount = 0;
            if (_isStuck) {
                // enable pairwise filter
                _physicsEngine->setContactAddedCallback(applyPairwiseFilter);
                _pairwiseFilter.incrementStepCount();
            } else {
                // disable pairwise filter
                _physicsEngine->setContactAddedCallback(nullptr);
                _appliedStuckRecoveryStrategy = false;
                _pairwiseFilter.clearAllEntries();
            }
            updateCurrentGravity();
        }
    } else {
        _stuckTransitionCount = 0;
        if (_isStuck) {
            ++_numStuckSubsteps;
            _appliedStuckRecoveryStrategy = false;
        }
    }
    return hasFloor;
}

void CharacterController::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) {
    preStep(collisionWorld);
    playerStep(collisionWorld, deltaTime);
}

void CharacterController::preStep(btCollisionWorld* collisionWorld) {
    // trace a ray straight down to see if we're standing on the ground
    const btTransform& transform = _rigidBody->getWorldTransform();

    // rayStart is at center of bottom sphere
    btVector3 rayStart = transform.getOrigin() - _halfHeight * _currentUp;

    // rayEnd is some short distance outside bottom sphere
    const btScalar FLOOR_PROXIMITY_THRESHOLD = 0.3f * _radius;
    btScalar rayLength = _radius + FLOOR_PROXIMITY_THRESHOLD;
    btVector3 rayEnd = rayStart - rayLength * _currentUp;

    // scan down for nearby floor
    ClosestNotMe rayCallback(_rigidBody);
    rayCallback.m_closestHitFraction = 1.0f;
    collisionWorld->rayTest(rayStart, rayEnd, rayCallback);
    if (rayCallback.hasHit()) {
        _floorDistance = rayLength * rayCallback.m_closestHitFraction - _radius;
    }
}

const btScalar MIN_TARGET_SPEED = 0.001f;
const btScalar MIN_TARGET_SPEED_SQUARED = MIN_TARGET_SPEED * MIN_TARGET_SPEED;

void CharacterController::playerStep(btCollisionWorld* collisionWorld, btScalar dt) {
    _stepHeight = _minStepHeight; // clears memory of last step obstacle
    _hasSupport = checkForSupport(collisionWorld);
    btVector3 velocity = _rigidBody->getLinearVelocity() - _parentVelocity;
    computeNewVelocity(dt, velocity);

    constexpr float MINIMUM_TIME_REMAINING = 0.005f;
    static_assert(FOLLOW_TIME_IMMEDIATE_SNAP > MINIMUM_TIME_REMAINING, "The code below assumes this condition is true.");

    bool hasFollowTimeRemaining = false;
    for (float followTime : _followTimeRemainingPerType) {
        if (followTime > MINIMUM_TIME_REMAINING) {
            hasFollowTimeRemaining = true;
            break;
        }
    }

    if (hasFollowTimeRemaining) {
        const float MAX_DISPLACEMENT = 0.5f * _radius;

        btTransform bodyTransform = _rigidBody->getWorldTransform();
        btVector3 startPos = bodyTransform.getOrigin();
        btVector3 deltaPos = _followDesiredBodyTransform.getOrigin() - startPos;

        btVector3 linearDisplacement(0.0f, 0.0f, 0.0f);
        {
            float horizontalTime = _followTimeRemainingPerType[static_cast<uint>(FollowType::Horizontal)];
            float verticalTime = _followTimeRemainingPerType[static_cast<uint>(FollowType::Vertical)];

            if (horizontalTime == FOLLOW_TIME_IMMEDIATE_SNAP) {
                linearDisplacement.setX(deltaPos.x());
                linearDisplacement.setZ(deltaPos.z());
            } else if (horizontalTime > MINIMUM_TIME_REMAINING) {
                linearDisplacement.setX((deltaPos.x() * dt) / horizontalTime);
                linearDisplacement.setZ((deltaPos.z() * dt) / horizontalTime);
            }

            if (verticalTime == FOLLOW_TIME_IMMEDIATE_SNAP) {
                linearDisplacement.setY(deltaPos.y());
            } else if (verticalTime > MINIMUM_TIME_REMAINING) {
                linearDisplacement.setY((deltaPos.y() * dt) / verticalTime);
            }

            linearDisplacement = clampLength(linearDisplacement, MAX_DISPLACEMENT);  // clamp displacement to prevent tunneling.
        }

        btVector3 endPos = startPos + linearDisplacement;

        // resolve the simple linearDisplacement
        _followLinearDisplacement += linearDisplacement;

        // now for the rotational part...

        btQuaternion startRot = bodyTransform.getRotation();

        // startRot as default rotation
        btQuaternion endRot = startRot;

        float rotationTime = _followTimeRemainingPerType[static_cast<uint>(FollowType::Rotation)];
        if (rotationTime > MINIMUM_TIME_REMAINING) {
            btQuaternion desiredRot = _followDesiredBodyTransform.getRotation();

            // the dot product between two quaternions is equal to +/- cos(angle/2)
            // where 'angle' is that of the rotation between them
            float qDot = desiredRot.dot(startRot);

            // when the abs() value of the dot product is approximately 1.0
            // then the two rotations are effectively adjacent
            const float MIN_DOT_PRODUCT_OF_ADJACENT_QUATERNIONS = 0.99999f;  // corresponds to approx 0.5 degrees
            if (fabsf(qDot) < MIN_DOT_PRODUCT_OF_ADJACENT_QUATERNIONS) {
                if (qDot < 0.0f) {
                    // the quaternions are actually on opposite hyperhemispheres
                    // so we move one to agree with the other and negate qDot
                    desiredRot = -desiredRot;
                    qDot = -qDot;
                }
                btQuaternion deltaRot = desiredRot * startRot.inverse();

                // the axis is the imaginary part, but scaled by sin(angle/2)
                btVector3 axis(deltaRot.getX(), deltaRot.getY(), deltaRot.getZ());
                axis /= sqrtf(1.0f - qDot * qDot);

                // compute the angle we will resolve for this dt, but don't overshoot
                float angle = 2.0f * acosf(qDot);

                if (rotationTime != FOLLOW_TIME_IMMEDIATE_SNAP) {
                    if (dt < rotationTime) {
                        angle *= dt / rotationTime;
                    }
                }

                // accumulate rotation
                deltaRot = btQuaternion(axis, angle);
                _followAngularDisplacement = (deltaRot * _followAngularDisplacement).normalize();

                // in order to accumulate displacement of avatar position, we need to take _shapeLocalOffset into account.
                btVector3 shapeLocalOffset = glmToBullet(_shapeLocalOffset);

                endRot = deltaRot * startRot;
                btVector3 swingDisplacement =
                    rotateVector(endRot, -shapeLocalOffset) - rotateVector(startRot, -shapeLocalOffset);
                _followLinearDisplacement += swingDisplacement;
            }
        }
        _rigidBody->setWorldTransform(btTransform(endRot, endPos));
    }
    _followTime += dt;

    if (_steppingUp) {
        float horizontalTargetSpeed = (_targetVelocity - _targetVelocity.dot(_currentUp) * _currentUp).length();
        if (horizontalTargetSpeed > FLT_EPSILON) {
            // compute a stepUpSpeed that will reach the top of the step in the time it would take
            // to move over the _stepPoint at target speed
            float horizontalDistance = (_stepPoint - _stepPoint.dot(_currentUp) * _currentUp).length();
            float timeToStep = horizontalDistance / horizontalTargetSpeed;
            float stepUpSpeed = _stepHeight / timeToStep;

            // magically clamp stepUpSpeed to a fraction of horizontalTargetSpeed
            // to prevent the avatar from moving unreasonably fast according to human eye
            const float MAX_STEP_UP_SPEED = 0.65f * horizontalTargetSpeed;
            if (stepUpSpeed > MAX_STEP_UP_SPEED) {
                stepUpSpeed = MAX_STEP_UP_SPEED;
            }

            // add minimum velocity to counteract gravity's displacement during one step
            // Note: the 0.5 factor comes from the fact that we really want the
            // average velocity contribution from gravity during the step
            stepUpSpeed -= 0.5f * _currentGravity * timeToStep; // remember: _gravity is negative scalar

            btScalar vDotUp = velocity.dot(_currentUp);
            if (vDotUp < stepUpSpeed) {
                // character doesn't have enough upward velocity to cover the step so we help using a "sky hook"
                // which uses micro-teleports rather than applying real velocity
                // to prevent the avatar from popping up after the step is done
                btTransform transform = _rigidBody->getWorldTransform();
                transform.setOrigin(transform.getOrigin() + (dt * stepUpSpeed) * _currentUp);
                _rigidBody->setWorldTransform(transform);
            }

            // don't allow the avatar to fall downward when stepping up
            // since otherwise this would tend to defeat the step-up behavior
            if (vDotUp < 0.0f) {
                velocity -= vDotUp * _currentUp;
            }
        }
    }
    _rigidBody->setLinearVelocity(velocity + _parentVelocity);
    _ghost.setWorldTransform(_rigidBody->getWorldTransform());
}

void CharacterController::jump(const btVector3& dir) {
    _pendingFlags |= PENDING_FLAG_JUMP;
}

bool CharacterController::onGround() const {
    const btScalar FLOOR_PROXIMITY_THRESHOLD = 0.3f * _radius;
    return _floorDistance < FLOOR_PROXIMITY_THRESHOLD || _hasSupport;
}

#ifdef DEBUG_STATE_CHANGE
static const char* stateToStr(CharacterController::State state) {
    switch (state) {
    case CharacterController::State::Ground:
        return "Ground";
    case CharacterController::State::Takeoff:
        return "Takeoff";
    case CharacterController::State::InAir:
        return "InAir";
    case CharacterController::State::Hover:
        return "Hover";
    case CharacterController::State::Seated:
        return "Seated";
    default:
        return "Unknown";
    }
}
#endif // #ifdef DEBUG_STATE_CHANGE

void CharacterController::updateCurrentGravity() {
    int32_t collisionMask = computeCollisionMask();
    if (_state == State::Hover || collisionMask == BULLET_COLLISION_MASK_COLLISIONLESS || _isStuck) {
        _currentGravity = 0.0f;
    } else {
        _currentGravity = _gravity;
    }
    if (_rigidBody) {
        _rigidBody->setGravity(_currentGravity * _currentUp);
    }
}


void CharacterController::setGravity(float gravity) {
    _gravity = gravity;
    updateCurrentGravity();
}

float CharacterController::getGravity() {
    return _gravity;
}

#ifdef DEBUG_STATE_CHANGE
void CharacterController::setState(State desiredState, const char* reason) {
#else
void CharacterController::setState(State desiredState) {
#endif

    if (desiredState != _state) {
#ifdef DEBUG_STATE_CHANGE
        qCDebug(physics) << "CharacterController::setState" << stateToStr(desiredState) << "from" << stateToStr(_state) << "," << reason;
#endif
        _state = desiredState;
        updateCurrentGravity();
    }
}

void CharacterController::recomputeFlying() {
    _pendingFlags |= PENDING_FLAG_RECOMPUTE_FLYING;
}

void CharacterController::setLocalBoundingBox(const glm::vec3& minCorner, const glm::vec3& scale) {
    float x = scale.x;
    float z = scale.z;
    float radius = 0.5f * sqrtf(0.5f * (x * x + z * z));
    float halfHeight = 0.5f * scale.y - radius;
    float MIN_HALF_HEIGHT = 0.0f;
    if (halfHeight < MIN_HALF_HEIGHT) {
        halfHeight = MIN_HALF_HEIGHT;
    }

    // compare dimensions
    if (glm::abs(radius - _radius) > FLT_EPSILON || glm::abs(halfHeight - _halfHeight) > FLT_EPSILON) {
        _radius = radius;
        _halfHeight = halfHeight;
        const btScalar DEFAULT_MIN_STEP_HEIGHT_FACTOR = 0.005f;
        const btScalar DEFAULT_MAX_STEP_HEIGHT_FACTOR = 0.65f;
        _minStepHeight = DEFAULT_MIN_STEP_HEIGHT_FACTOR * (_halfHeight + _radius);
        _maxStepHeight = DEFAULT_MAX_STEP_HEIGHT_FACTOR * (_halfHeight + _radius);

        _pendingFlags |= PENDING_FLAG_UPDATE_SHAPE | PENDING_FLAG_REMOVE_FROM_SIMULATION | PENDING_FLAG_REMOVE_DETAILED_FROM_SIMULATION |
            PENDING_FLAG_ADD_TO_SIMULATION | PENDING_FLAG_ADD_DETAILED_TO_SIMULATION;
    }

    // it's ok to change offset immediately -- there are no thread safety issues here
    _shapeLocalOffset = minCorner + 0.5f * scale;

    if (_rigidBody) {
        // update CCD with new _radius
        _rigidBody->setCcdSweptSphereRadius(_halfHeight);
        _rigidBody->setCcdMotionThreshold(_radius);
    }
}

void CharacterController::setPhysicsEngine(const PhysicsEnginePointer& engine) {
    if (!_physicsEngine && engine) {
        // ATM there is only one PhysicsEngine: it is a singleton, and we are taking advantage
        // of that assumption here.  If we ever introduce more and allow for this backpointer
        // to change then we'll have to overhaul this method.
        _physicsEngine = engine;
    }
}

float CharacterController::getCollisionBrakeAttenuationFactor() const {
    // _collisionBrake ranges from 0.0 (no brake) to 1.0 (max brake)
    // which we use to compute a corresponding attenutation factor from 1.0 to 0.5
    return 1.0f - 0.5f * _collisionBrake;
}

void CharacterController::setCollisionless(bool collisionless) {
    if (collisionless != _collisionless) {
        _collisionless = collisionless;
        _pendingFlags |= PENDING_FLAG_UPDATE_COLLISION_MASK;
    }
}

void CharacterController::updateUpAxis(const glm::quat& rotation) {
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    if (_rigidBody) {
        _rigidBody->setGravity(_currentGravity * _currentUp);
    }
}

void CharacterController::setPositionAndOrientation(
        const glm::vec3& position,
        const glm::quat& orientation) {
    updateUpAxis(orientation);
    _rotation = glmToBullet(orientation);
    _position = glmToBullet(position + orientation * _shapeLocalOffset);
}

void CharacterController::getPositionAndOrientation(glm::vec3& position, glm::quat& rotation) const {
    if (_rigidBody) {
        const btTransform& avatarTransform = _rigidBody->getWorldTransform();
        rotation = bulletToGLM(avatarTransform.getRotation());
        position = bulletToGLM(avatarTransform.getOrigin()) - rotation * _shapeLocalOffset;
    }
}

void CharacterController::setParentVelocity(const glm::vec3& velocity) {
    _parentVelocity = glmToBullet(velocity);
}

void CharacterController::setFollowParameters(const glm::mat4& desiredWorldBodyMatrix) {
    _followDesiredBodyTransform = glmToBullet(desiredWorldBodyMatrix) * btTransform(btQuaternion::getIdentity(), glmToBullet(_shapeLocalOffset));
}

glm::vec3 CharacterController::getFollowLinearDisplacement() const {
    return bulletToGLM(_followLinearDisplacement);
}

glm::quat CharacterController::getFollowAngularDisplacement() const {
    return bulletToGLM(_followAngularDisplacement);
}

glm::vec3 CharacterController::getFollowVelocity() const {
    if (_followTime > 0.0f) {
        return bulletToGLM(_followLinearDisplacement) / _followTime;
    } else {
        return glm::vec3();
    }
}

glm::vec3 CharacterController::getLinearVelocity() const {
    glm::vec3 velocity(0.0f);
    if (_rigidBody) {
        velocity = bulletToGLM(_rigidBody->getLinearVelocity());
    }
    return velocity;
}

glm::vec3 CharacterController::getVelocityChange() const {
    if (_rigidBody) {
        return bulletToGLM(_velocityChange);
    }
    return glm::vec3(0.0f);
}

void CharacterController::clearMotors() {
    _motors.clear();
}

void CharacterController::addMotor(const glm::vec3& velocity, const glm::quat& rotation, float horizTimescale, float vertTimescale) {
    _motors.push_back(CharacterController::CharacterMotor(velocity, rotation, horizTimescale, vertTimescale));
}

void CharacterController::applyMotor(int index, btScalar dt, btVector3& worldVelocity, std::vector<btVector3>& velocities, std::vector<btScalar>& weights) {
    assert(index < (int)(_motors.size()));
    CharacterController::CharacterMotor& motor = _motors[index];
    if (motor.hTimescale >= MAX_CHARACTER_MOTOR_TIMESCALE && motor.vTimescale >= MAX_CHARACTER_MOTOR_TIMESCALE) {
        // nothing to do
        return;
    }

    const bool motorHasRotation = !(motor.rotation == btQuaternion::getIdentity());
    btVector3 axis = motor.rotation.getAxis();
    btScalar angle = motor.rotation.getAngle();

	// Rotate a vector from motor frame to world frame
    auto rotateToWorldFrame = [&axis, &angle, &motorHasRotation](const btVector3 vectorInMotorFrame) {
        if (motorHasRotation) {
            return vectorInMotorFrame.rotate(axis, angle);
        } else {
            return vectorInMotorFrame;
        }
    };

	// Rotate a vector from world frame to motor frame
    auto rotateToMotorFrame = [&axis, &angle, &motorHasRotation](const btVector3 vectorInWorldFrame) {
        if (motorHasRotation) {
            return vectorInWorldFrame.rotate(axis, -angle);
        } else {
            return vectorInWorldFrame;
        }
    };

    btVector3 velocity = rotateToMotorFrame(worldVelocity);

    int32_t collisionMask = computeCollisionMask();
    if (collisionMask == BULLET_COLLISION_MASK_COLLISIONLESS ||
            _state == State::Hover || motor.hTimescale == motor.vTimescale) {
        // modify velocity
        btScalar tau = dt / motor.hTimescale;
        if (tau > 1.0f) {
            tau = 1.0f;
        }
        velocity += tau * (motor.velocity - velocity);

        // rotate back into world-frame
        velocity = rotateToWorldFrame(velocity);
        _targetVelocity += rotateToWorldFrame(tau * motor.velocity);

        // store the velocity and weight
        velocities.push_back(velocity);
        weights.push_back(tau);
    } else {
        // compute local UP
        btVector3 up = rotateToMotorFrame(_currentUp);
        btVector3 motorVelocity = motor.velocity;

        // save these non-adjusted components for later
        btVector3 vTargetVelocity = motorVelocity.dot(up) * up;
        btVector3 hTargetVelocity = motorVelocity - vTargetVelocity;

        if (_stepHeight > _minStepHeight && !_steppingUp) {
            // there is a step --> compute velocity direction to go over step
            btVector3 motorVelocityWF = rotateToWorldFrame(motorVelocity);
            if (motorVelocityWF.dot(_stepNormal) < 0.0f) {
                // the motor pushes against step
                _steppingUp = true;
            }
        }

        // split velocity into horizontal and vertical components
        btVector3 vVelocity = velocity.dot(up) * up;
        btVector3 hVelocity = velocity - vVelocity;
        btVector3 vMotorVelocity = motorVelocity.dot(up) * up;
        btVector3 hMotorVelocity = motorVelocity - vMotorVelocity;

        // modify each component separately
        btScalar maxTau = 0.0f;
        if (motor.hTimescale < MAX_CHARACTER_MOTOR_TIMESCALE) {
            btScalar tau = dt / motor.hTimescale;
            if (tau > 1.0f) {
                tau = 1.0f;
            }
            maxTau = tau;
            hVelocity += (hMotorVelocity - hVelocity) * tau;
        }
        if (motor.vTimescale < MAX_CHARACTER_MOTOR_TIMESCALE) {
            btScalar tau = dt / motor.vTimescale;
            if (tau > 1.0f) {
                tau = 1.0f;
            }
            if (tau > maxTau) {
                maxTau = tau;
            }
            vVelocity += (vMotorVelocity - vVelocity) * tau;
        }

        // add components back together and rotate into world-frame
        velocity = rotateToWorldFrame(hVelocity + vVelocity);
        _targetVelocity += maxTau * rotateToWorldFrame(hTargetVelocity + vTargetVelocity);

        // store velocity and weights
        velocities.push_back(velocity);
        weights.push_back(maxTau);
    }
}

void CharacterController::computeNewVelocity(btScalar dt, btVector3& velocity) {
    btVector3 currentVelocity = velocity;

    if (velocity.length2() < MIN_TARGET_SPEED_SQUARED) {
        velocity = btVector3(0.0f, 0.0f, 0.0f);
    }

    // measure velocity changes and their weights
    std::vector<btVector3> velocities;
    velocities.reserve(_motors.size());
    std::vector<btScalar> weights;
    weights.reserve(_motors.size());
    _targetVelocity = btVector3(0.0f, 0.0f, 0.0f);
    _steppingUp = false;
    for (int i = 0; i < (int)_motors.size(); ++i) {
        applyMotor(i, dt, velocity, velocities, weights);
    }
    assert(velocities.size() == weights.size());

    // blend velocity changes according to relative weights
    btScalar totalWeight = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        totalWeight += weights[i];
    }
    if (totalWeight > 0.0f) {
        velocity = btVector3(0.0f, 0.0f, 0.0f);
        for (size_t i = 0; i < velocities.size(); ++i) {
            velocity += (weights[i] / totalWeight) * velocities[i];
        }
        _targetVelocity /= totalWeight;
    }
    if (velocity.length2() < MIN_TARGET_SPEED_SQUARED) {
        velocity = btVector3(0.0f, 0.0f, 0.0f);
    }
    if (_isStuck && _targetVelocity.length2() < MIN_TARGET_SPEED_SQUARED) {
        // we're stuck, but not trying to move --> move UP by default
        // in the hopes we'll get unstuck
        const float STUCK_EXTRACTION_SPEED = 1.0f;
        velocity = STUCK_EXTRACTION_SPEED * _currentUp;
    }

    // 'thrust' is applied at the very end
    _targetVelocity += dt * _linearAcceleration;
    velocity += dt * _linearAcceleration;
    // Note the differences between these two variables:
    // _targetVelocity = ideal final velocity according to input
    // velocity = new final velocity after motors are applied to currentVelocity

    // but we want to avoid getting stuck and tunelling through geometry so we perform
    // further checks and modify/abandon our velocity calculations

    const float SAFE_COLLISION_SPEED = glm::abs(STUCK_PENETRATION) * (float)NUM_SUBSTEPS_PER_SECOND;
    const float SAFE_COLLISION_SPEED_SQUARED = SAFE_COLLISION_SPEED * SAFE_COLLISION_SPEED;

    // NOTE: the thresholds are negative which indicates the vectors oppose each other
    // and which means comparison operators against them may look wrong at first glance.
    // The magnitudes of the thresholds have been tuned manually.
    const float STRONG_OPPOSING_IMPACT_THRESHOLD = -1000.0f;
    const float VERY_STRONG_OPPOSING_IMPACT_THRESHOLD = -2000.0f;
    float velocityDotImpulse = velocity.dot(_netCollisionImpulse);

    const float COLLISION_BRAKE_TIMESCALE = 0.20f; // must be > PHYSICS_ENGINE_FIXED_SUBSTEP for stability
    const float MIN_COLLISION_BRAKE = 0.05f;
    if ((velocityDotImpulse > VERY_STRONG_OPPOSING_IMPACT_THRESHOLD && _stuckTransitionCount == 0) || _isStuck) {
        // we are either definitely NOT stuck (in which case nothing to do)
        // or definitely are (in which case we'll be temporarily disabling collisions with the offending object
        // and we don't mind tunnelling as an escape route out of stuck)
        if (_collisionBrake > MIN_COLLISION_BRAKE) {
            _collisionBrake *= (1.0f - dt / COLLISION_BRAKE_TIMESCALE);
            if (_collisionBrake < MIN_COLLISION_BRAKE) {
                _collisionBrake = 0.0f;
            }
        }
        return;
    }

    if (velocityDotImpulse < VERY_STRONG_OPPOSING_IMPACT_THRESHOLD ||
            (velocityDotImpulse < STRONG_OPPOSING_IMPACT_THRESHOLD && velocity.length2() > SAFE_COLLISION_SPEED_SQUARED)) {
        if (_collisionBrake < 1.0f) {
            _collisionBrake += (1.0f - _collisionBrake) * (dt / COLLISION_BRAKE_TIMESCALE);
            const float MAX_COLLISION_BRAKE = 1.0f - MIN_COLLISION_BRAKE;
            if (_collisionBrake > MAX_COLLISION_BRAKE) {
                _collisionBrake = 1.0f;
            }
        }

        // NOTE about REFLECTION_COEFFICIENT: a value of 2.0 provides full reflection
        // (zero attenuation) whereas a value of 1.0 zeros it (full attenuation).
        const float REFLECTION_COEFFICIENT = 1.1f;

        if (velocity.dot(currentVelocity) > 0.0f) {
            // our new velocity points in the same direction as our currentVelocity
            // but negative "impact" means new velocity points against netCollisionImpulse
            if (currentVelocity.dot(_netCollisionImpulse) > 0.0f) {
                // currentVelocity points positively with netCollisionImpulse --> trust physics to save us
                velocity = currentVelocity;
            } else {
                // can't trust physics --> use new velocity but reflect it
                btVector3 impulseDirection = _netCollisionImpulse.normalized();
                velocity -= (REFLECTION_COEFFICIENT * velocity.dot(impulseDirection)) * impulseDirection;
            }
        } else {
            // currentVelocity points against new velocity, which means it is probably better but...
            // when the physical simulation starts to fail (e.g. in deep penetration in mesh geometry)
            // the currentVelocity can point in unhelpful directions, so we check it and reflect any component
            // opposing netCollisionImpulse in hopes netCollisionImpulse points toward good exit
            if (currentVelocity.dot(_netCollisionImpulse) < 0.0f) {
                // currentVelocity points against netCollisionImpulse --> reflect
                btVector3 impulseDirection = _netCollisionImpulse.normalized();
                currentVelocity -= (REFLECTION_COEFFICIENT * currentVelocity.dot(impulseDirection)) * impulseDirection;
            }
            velocity = currentVelocity;
        }
    }
}

void CharacterController::computeNewVelocity(btScalar dt, glm::vec3& velocity) {
    btVector3 btVelocity = glmToBullet(velocity);
    computeNewVelocity(dt, btVelocity);
    velocity = bulletToGLM(btVelocity);
}

void CharacterController::updateState() {
    if (!_physicsEngine) {
        return;
    }
    if (_pendingFlags & PENDING_FLAG_RECOMPUTE_FLYING) {
         SET_STATE(CharacterController::State::Hover, "recomputeFlying");
         _hasSupport = false;
         _stepHeight = _minStepHeight; // clears memory of last step obstacle
         _pendingFlags &= ~PENDING_FLAG_RECOMPUTE_FLYING;
    }

    const btScalar FLY_TO_GROUND_THRESHOLD = 0.1f * _radius;
    const btScalar GROUND_TO_FLY_THRESHOLD = 0.8f * _radius + _halfHeight;
    const quint64 TAKE_OFF_TO_IN_AIR_PERIOD = 250 * MSECS_PER_SECOND;
    const btScalar MIN_HOVER_HEIGHT = _scaleFactor * DEFAULT_AVATAR_MIN_HOVER_HEIGHT;
    const quint64 JUMP_TO_HOVER_PERIOD = _scaleFactor < 1.0f ? _scaleFactor * 1100 * MSECS_PER_SECOND : 1100 * MSECS_PER_SECOND;

    // scan for distant floor
    // rayStart is at center of bottom sphere
    btVector3 rayStart = _position;

    btScalar rayLength = _radius;
    int32_t collisionMask = computeCollisionMask();
    if (collisionMask == BULLET_COLLISION_MASK_COLLISIONLESS) {
        rayLength += MIN_HOVER_HEIGHT;
    } else {
        rayLength += _scaleFactor * DEFAULT_AVATAR_FALL_HEIGHT;
    }
    btVector3 rayEnd = rayStart - rayLength * _currentUp;

    ClosestNotMe rayCallback(_rigidBody);
    rayCallback.m_closestHitFraction = 1.0f;
    _physicsEngine->getDynamicsWorld()->rayTest(rayStart, rayEnd, rayCallback);
    bool rayHasHit = rayCallback.hasHit();
    quint64 now = usecTimestampNow();
    if (rayHasHit) {
        _rayHitStartTime = now;
        _floorDistance = rayLength * rayCallback.m_closestHitFraction - (_radius + _halfHeight);
    } else {
        const quint64 RAY_HIT_START_PERIOD = 500 * MSECS_PER_SECOND;
        if ((now - _rayHitStartTime) < RAY_HIT_START_PERIOD) {
            rayHasHit = true;
        } else {
            _floorDistance = FLT_MAX;
        }
    }

    // record a time stamp when the jump button was first pressed.
    bool jumpButtonHeld = _pendingFlags & PENDING_FLAG_JUMP;
    if ((_previousFlags & PENDING_FLAG_JUMP) != (_pendingFlags & PENDING_FLAG_JUMP)) {
        if (_pendingFlags & PENDING_FLAG_JUMP) {
            _jumpButtonDownStartTime = now;
            _jumpButtonDownCount++;
        }
    }

    btVector3 velocity = _preSimulationVelocity;

    // disable normal state transitions while collisionless
    const btScalar MAX_WALKING_SPEED = 2.65f;
    if (collisionMask == BULLET_COLLISION_MASK_COLLISIONLESS) {
        // when collisionless: only switch between State::Ground, State::Hover and State::Seated
        // and bypass state debugging
        if (_isSeated) {
            _state = State::Seated;
        } else if (rayHasHit) {
            if (velocity.length() > (MAX_WALKING_SPEED)) {
                _state = State::Hover;
            } else {
                _state = State::Ground;
            }
        } else {
            _state = State::Hover;
        }
    } else {
        switch (_state) {
            case State::Ground:
                if (!rayHasHit && !_hasSupport) {
                    if (_hoverWhenUnsupported) {
                       SET_STATE(State::Hover, "no ground detected");
                    } else {
                       SET_STATE(State::InAir, "falling");
                    }
                } else if (_pendingFlags & PENDING_FLAG_JUMP && _jumpButtonDownCount != _takeoffJumpButtonID) {
                    _takeoffJumpButtonID = _jumpButtonDownCount;
                    _takeoffToInAirStartTime = now;
                    SET_STATE(State::Takeoff, "jump pressed");
                } else if (rayHasHit && !_hasSupport && _floorDistance > GROUND_TO_FLY_THRESHOLD) {
                    SET_STATE(State::InAir, "falling");
                }
                break;
            case State::Takeoff:
                if (_hoverWhenUnsupported && (!rayHasHit && !_hasSupport)) {
                    SET_STATE(State::Hover, "no ground");
                } else if ((now - _takeoffToInAirStartTime) > TAKE_OFF_TO_IN_AIR_PERIOD) {
                    SET_STATE(State::InAir, "takeoff done");

                    // compute jumpSpeed based on the scaled jump height for the default avatar in default gravity.
                    const float jumpHeight = std::max(_scaleFactor * DEFAULT_AVATAR_JUMP_HEIGHT, DEFAULT_AVATAR_MIN_JUMP_HEIGHT);
                    const float jumpSpeed = sqrtf(2.0f * -DEFAULT_AVATAR_GRAVITY * jumpHeight);
                    velocity += jumpSpeed * _currentUp;
                    _rigidBody->setLinearVelocity(velocity);
                }
                break;
            case State::InAir: {
                const float jumpHeight = std::max(_scaleFactor * DEFAULT_AVATAR_JUMP_HEIGHT, DEFAULT_AVATAR_MIN_JUMP_HEIGHT);
                const float jumpSpeed = sqrtf(2.0f * -DEFAULT_AVATAR_GRAVITY * jumpHeight);
                if ((velocity.dot(_currentUp) <= (jumpSpeed / 2.0f)) && ((_floorDistance < FLY_TO_GROUND_THRESHOLD) || _hasSupport)) {
                    SET_STATE(State::Ground, "hit ground");
                } else if (_zoneFlyingAllowed) {
                    btVector3 desiredVelocity = _targetVelocity;
                    if (desiredVelocity.length2() < MIN_TARGET_SPEED_SQUARED) {
                        desiredVelocity = btVector3(0.0f, 0.0f, 0.0f);
                    }
                    bool vertTargetSpeedIsNonZero = desiredVelocity.dot(_currentUp) > MIN_TARGET_SPEED;
                    if (_comfortFlyingAllowed && (jumpButtonHeld || vertTargetSpeedIsNonZero) && (_takeoffJumpButtonID != _jumpButtonDownCount)) {
                        SET_STATE(State::Hover, "double jump button");
                    } else if (_comfortFlyingAllowed && (jumpButtonHeld || vertTargetSpeedIsNonZero) && (now - _jumpButtonDownStartTime) > JUMP_TO_HOVER_PERIOD) {
                        SET_STATE(State::Hover, "jump button held");
                    } else if (_hoverWhenUnsupported && ((!rayHasHit && !_hasSupport) || _floorDistance > _scaleFactor * DEFAULT_AVATAR_FALL_HEIGHT)) {
                        // Transition to hover if there's no ground beneath us or we are above the fall threshold, regardless of _comfortFlyingAllowed
                        SET_STATE(State::Hover, "above fall threshold");
                    }
                }
                break;
            }
            case State::Hover: {
                btScalar horizontalSpeed = (velocity - velocity.dot(_currentUp) * _currentUp).length();
                bool flyingFast = horizontalSpeed > (MAX_WALKING_SPEED * 0.75f);
                if (!_zoneFlyingAllowed) {
                    SET_STATE(State::InAir, "zone flying not allowed");
                } else if (!_comfortFlyingAllowed && (rayHasHit || _hasSupport || _floorDistance < FLY_TO_GROUND_THRESHOLD)) {
                    SET_STATE(State::InAir, "comfort flying not allowed");
                } else if ((_floorDistance < MIN_HOVER_HEIGHT) && !jumpButtonHeld && !flyingFast) {
                    SET_STATE(State::InAir, "near ground");
                } else if (((_floorDistance < FLY_TO_GROUND_THRESHOLD) || _hasSupport) && !flyingFast) {
                    SET_STATE(State::Ground, "touching ground");
                }
                break;
            }
            case State::Seated: {
                SET_STATE(State::Ground, "Standing up");
                break;
            }
        }
    }
}

void CharacterController::preSimulation() {
    if (needsRemoval()) {
        removeFromWorld();

        // We must remove any existing contacts for the avatar so that any new contacts will have
        // valid data.  MyAvatar's RigidBody is the ONLY one in the simulation that does not yet
        // have a MotionState so we pass nullptr to removeContacts().
        if (_physicsEngine) {
            _physicsEngine->removeContacts(nullptr);
        }
    }
    updateShapeIfNecessary();
    if (needsAddition()) {
        addToWorld();
    }

    if (_rigidBody) {
        // slam body transform and remember velocity
        _rigidBody->setWorldTransform(btTransform(btTransform(_rotation, _position)));
        _preSimulationVelocity = _rigidBody->getLinearVelocity();

        updateState();
    }

    _previousFlags = _pendingFlags;
    _pendingFlags &= ~PENDING_FLAG_JUMP;

    _followTime = 0.0f;
    _followLinearDisplacement = btVector3(0, 0, 0);
    _followAngularDisplacement = btQuaternion::getIdentity();

    if (_isStuck) {
        _pairwiseFilter.expireOldEntries();
        _pairwiseFilter.incrementStepCount();
    }
}

void CharacterController::postSimulation() {
    if (_rigidBody) {
        _velocityChange = _rigidBody->getLinearVelocity() - _preSimulationVelocity;
    }
}

bool CharacterController::getRigidBodyLocation(glm::vec3& avatarRigidBodyPosition, glm::quat& avatarRigidBodyRotation) {
    if (!_rigidBody) {
        return false;
    }

    const btTransform& worldTrans = _rigidBody->getCenterOfMassTransform();
    avatarRigidBodyPosition = bulletToGLM(worldTrans.getOrigin()) + ObjectMotionState::getWorldOffset();
    avatarRigidBodyRotation = bulletToGLM(worldTrans.getRotation());
    return true;
}

void CharacterController::setCollisionlessAllowed(bool value) {
    if (value != _collisionlessAllowed) {
        _collisionlessAllowed = value;
        _pendingFlags |= PENDING_FLAG_UPDATE_COLLISION_MASK;
    }
}
