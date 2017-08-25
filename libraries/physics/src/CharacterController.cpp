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

#include <NumericalConstants.h>
#include <AvatarConstants.h>

#include "ObjectMotionState.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"

const btVector3 LOCAL_UP_AXIS(0.0f, 1.0f, 0.0f);

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

CharacterController::CharacterController() {
    _floorDistance = _scaleFactor * DEFAULT_AVATAR_FALL_HEIGHT;

    _targetVelocity.setValue(0.0f, 0.0f, 0.0f);
    _followDesiredBodyTransform.setIdentity();
    _followTimeRemaining = 0.0f;
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
    return ((_pendingFlags & PENDING_FLAG_REMOVE_FROM_SIMULATION) == PENDING_FLAG_REMOVE_FROM_SIMULATION);
}

bool CharacterController::needsAddition() const {
    return ((_pendingFlags & PENDING_FLAG_ADD_TO_SIMULATION) == PENDING_FLAG_ADD_TO_SIMULATION);
}

void CharacterController::setDynamicsWorld(btDynamicsWorld* world) {
    if (_dynamicsWorld != world) {
        // remove from old world
        if (_dynamicsWorld) {
            if (_rigidBody) {
                _dynamicsWorld->removeRigidBody(_rigidBody);
                _dynamicsWorld->removeAction(this);
            }
            _dynamicsWorld = nullptr;
        }
        int16_t collisionGroup = computeCollisionGroup();
        if (_rigidBody) {
            updateMassProperties();
        }
        if (world && _rigidBody) {
            // add to new world
            _dynamicsWorld = world;
            _pendingFlags &= ~PENDING_FLAG_JUMP;
            _dynamicsWorld->addRigidBody(_rigidBody, collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR);
            _dynamicsWorld->addAction(this);
            // restore gravity settings because adding an object to the world overwrites its gravity setting
            _rigidBody->setGravity(_gravity * _currentUp);
            btCollisionShape* shape = _rigidBody->getCollisionShape();
            assert(shape && shape->getShapeType() == CONVEX_HULL_SHAPE_PROXYTYPE);
            _ghost.setCharacterShape(static_cast<btConvexHullShape*>(shape));
        }
        _ghost.setCollisionGroupAndMask(collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR & (~ collisionGroup));
        _ghost.setCollisionWorld(_dynamicsWorld);
        _ghost.setRadiusAndHalfHeight(_radius, _halfHeight);
        if (_rigidBody) {
            _ghost.setWorldTransform(_rigidBody->getWorldTransform());
        }
    }
    if (_dynamicsWorld) {
        if (_pendingFlags & PENDING_FLAG_UPDATE_SHAPE) {
            // shouldn't fall in here, but if we do make sure both ADD and REMOVE bits are still set
            _pendingFlags |= PENDING_FLAG_ADD_TO_SIMULATION | PENDING_FLAG_REMOVE_FROM_SIMULATION;
        } else {
            _pendingFlags &= ~PENDING_FLAG_ADD_TO_SIMULATION;
        }
    } else {
        _pendingFlags &= ~PENDING_FLAG_REMOVE_FROM_SIMULATION;
    }
}

bool CharacterController::checkForSupport(btCollisionWorld* collisionWorld) {
    bool pushing = _targetVelocity.length2() > FLT_EPSILON;

    btDispatcher* dispatcher = collisionWorld->getDispatcher();
    int numManifolds = dispatcher->getNumManifolds();
    bool hasFloor = false;
    bool isStuck = false;

    btTransform rotation = _rigidBody->getWorldTransform();
    rotation.setOrigin(btVector3(0.0f, 0.0f, 0.0f)); // clear translation part

    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
        if (_rigidBody == contactManifold->getBody1() || _rigidBody == contactManifold->getBody0()) {
            bool characterIsFirst = _rigidBody == contactManifold->getBody0();
            int numContacts = contactManifold->getNumContacts();
            int stepContactIndex = -1;
            float highestStep = _minStepHeight;
            for (int j = 0; j < numContacts; j++) {
                // check for "floor"
                btManifoldPoint& contact = contactManifold->getContactPoint(j);
                btVector3 pointOnCharacter = characterIsFirst ? contact.m_localPointA : contact.m_localPointB; // object-local-frame
                btVector3 normal = characterIsFirst ? contact.m_normalWorldOnB : -contact.m_normalWorldOnB; // points toward character
                btScalar hitHeight = _halfHeight + _radius + pointOnCharacter.dot(_currentUp);
                // If there's non-trivial penetration with a big impulse for several steps, we're probably stuck.
                // Note it here in the controller, and let MyAvatar figure out what to do about it.
                const float STUCK_PENETRATION = -0.05f; // always negative into the object.
                const float STUCK_IMPULSE = 500.0f;
                const int STUCK_LIFETIME = 3;
                if ((contact.getDistance() < STUCK_PENETRATION) && (contact.getAppliedImpulse() > STUCK_IMPULSE) && (contact.getLifeTime() > STUCK_LIFETIME)) {
                    isStuck = true; // latch on
                }
                if (hitHeight < _maxStepHeight && normal.dot(_currentUp) > _minFloorNormalDotUp) {
                    hasFloor = true;
                    if (!pushing && isStuck) {
                        // we're not pushing against anything and we're stuck so we can early exit
                        // (all we need to know is that there is a floor)
                        break;
                    }
                }
                if (pushing && _targetVelocity.dot(normal) < 0.0f) {
                    // remember highest step obstacle
                    if (!_stepUpEnabled || hitHeight > _maxStepHeight) {
                        // this manifold is invalidated by point that is too high
                        stepContactIndex = -1;
                        break;
                    } else if (hitHeight > highestStep && normal.dot(_targetVelocity) < 0.0f ) {
                        highestStep = hitHeight;
                        stepContactIndex = j;
                        hasFloor = true;
                    }
                }
            }
            if (stepContactIndex > -1 && highestStep > _stepHeight) {
                // remember step info for later
                btManifoldPoint& contact = contactManifold->getContactPoint(stepContactIndex);
                btVector3 pointOnCharacter = characterIsFirst ? contact.m_localPointA : contact.m_localPointB; // object-local-frame
                _stepNormal = characterIsFirst ? contact.m_normalWorldOnB : -contact.m_normalWorldOnB; // points toward character
                _stepHeight = highestStep;
                _stepPoint = rotation * pointOnCharacter; // rotate into world-frame
            }
            if (hasFloor && isStuck && !(pushing && _stepUpEnabled)) {
                // early exit since all we need to know is that we're on a floor
                break;
            }
        }
    }
    _isStuck = isStuck;
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

    const float MINIMUM_TIME_REMAINING = 0.005f;
    const float MAX_DISPLACEMENT = 0.5f * _radius;
    _followTimeRemaining -= dt;
    if (_followTimeRemaining >= MINIMUM_TIME_REMAINING) {
        btTransform bodyTransform = _rigidBody->getWorldTransform();

        btVector3 startPos = bodyTransform.getOrigin();
        btVector3 deltaPos = _followDesiredBodyTransform.getOrigin() - startPos;
        btVector3 vel = deltaPos / _followTimeRemaining;
        btVector3 linearDisplacement = clampLength(vel * dt, MAX_DISPLACEMENT);  // clamp displacement to prevent tunneling.
        btVector3 endPos = startPos + linearDisplacement;

        btQuaternion startRot = bodyTransform.getRotation();
        btQuaternion desiredRot = _followDesiredBodyTransform.getRotation();
        if (desiredRot.dot(startRot) < 0.0f) {
            desiredRot = -desiredRot;
        }
        btQuaternion deltaRot = desiredRot * startRot.inverse();
        float angularSpeed = deltaRot.getAngle() / _followTimeRemaining;
        btQuaternion angularDisplacement = btQuaternion(deltaRot.getAxis(), angularSpeed * dt);
        btQuaternion endRot = angularDisplacement * startRot;

        // in order to accumulate displacement of avatar position, we need to take _shapeLocalOffset into account.
        btVector3 shapeLocalOffset = glmToBullet(_shapeLocalOffset);
        btVector3 swingDisplacement = rotateVector(endRot, -shapeLocalOffset) - rotateVector(startRot, -shapeLocalOffset);

        _followLinearDisplacement = linearDisplacement + swingDisplacement + _followLinearDisplacement;
        _followAngularDisplacement = angularDisplacement * _followAngularDisplacement;

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
            stepUpSpeed -= 0.5f * _gravity * timeToStep; // remember: _gravity is negative scalar

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

void CharacterController::jump() {
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
    default:
        return "Unknown";
    }
}
#endif // #ifdef DEBUG_STATE_CHANGE

#ifdef DEBUG_STATE_CHANGE
void CharacterController::setState(State desiredState, const char* reason) {
#else
void CharacterController::setState(State desiredState) {
#endif
    if (!_flyingAllowed && desiredState == State::Hover) {
        desiredState = State::InAir;
    }

    if (desiredState != _state) {
#ifdef DEBUG_STATE_CHANGE
        qCDebug(physics) << "CharacterController::setState" << stateToStr(desiredState) << "from" << stateToStr(_state) << "," << reason;
#endif
        _state = desiredState;
        updateGravity();
    }
}

void CharacterController::updateGravity() {
    int16_t collisionGroup = computeCollisionGroup();
    if (_state == State::Hover || collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS) {
        _gravity = 0.0f;
    } else {
        _gravity = DEFAULT_AVATAR_GRAVITY;
    }
    if (_rigidBody) {
        _rigidBody->setGravity(_gravity * _currentUp);
    }
}

void CharacterController::setLocalBoundingBox(const glm::vec3& minCorner, const glm::vec3& scale) {
    float x = scale.x;
    float z = scale.z;
    float radius = 0.5f * sqrtf(0.5f * (x * x + z * z));
    float halfHeight = 0.5f * scale.y - radius;
    float MIN_HALF_HEIGHT = 0.1f;
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

        if (_dynamicsWorld) {
            // must REMOVE from world prior to shape update
            _pendingFlags |= PENDING_FLAG_REMOVE_FROM_SIMULATION;
        }
        _pendingFlags |= PENDING_FLAG_UPDATE_SHAPE;
        _pendingFlags |= PENDING_FLAG_ADD_TO_SIMULATION;
    }

    // it's ok to change offset immediately -- there are no thread safety issues here
    _shapeLocalOffset = minCorner + 0.5f * scale;
}

void CharacterController::setCollisionless(bool collisionless) {
    if (collisionless != _collisionless) {
        _collisionless = collisionless;
        _pendingFlags |= PENDING_FLAG_UPDATE_COLLISION_GROUP;
    }
}

int16_t CharacterController::computeCollisionGroup() const {
    if (_collisionless) {
        return _collisionlessAllowed ? BULLET_COLLISION_GROUP_COLLISIONLESS : BULLET_COLLISION_GROUP_MY_AVATAR;
    } else {
        return BULLET_COLLISION_GROUP_MY_AVATAR;
    }
}

void CharacterController::handleChangedCollisionGroup() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_COLLISION_GROUP) {
        // ATM the easiest way to update collision groups is to remove/re-add the RigidBody
        if (_dynamicsWorld) {
            _dynamicsWorld->removeRigidBody(_rigidBody);
            int16_t collisionGroup = computeCollisionGroup();
            _dynamicsWorld->addRigidBody(_rigidBody, collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR);
        }
        _pendingFlags &= ~PENDING_FLAG_UPDATE_COLLISION_GROUP;
        updateGravity();
    }
}

void CharacterController::updateUpAxis(const glm::quat& rotation) {
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    if (_rigidBody) {
        _rigidBody->setGravity(_gravity * _currentUp);
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

void CharacterController::setFollowParameters(const glm::mat4& desiredWorldBodyMatrix, float timeRemaining) {
    _followTimeRemaining = timeRemaining;
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

    // rotate into motor-frame
    btVector3 axis = motor.rotation.getAxis();
    btScalar angle = motor.rotation.getAngle();
    btVector3 velocity = worldVelocity.rotate(axis, -angle);

    int16_t collisionGroup = computeCollisionGroup();
    if (collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS ||
            _state == State::Hover || motor.hTimescale == motor.vTimescale) {
        // modify velocity
        btScalar tau = dt / motor.hTimescale;
        if (tau > 1.0f) {
            tau = 1.0f;
        }
        velocity += tau * (motor.velocity - velocity);

        // rotate back into world-frame
        velocity = velocity.rotate(axis, angle);
        _targetVelocity += (tau * motor.velocity).rotate(axis, angle);

        // store the velocity and weight
        velocities.push_back(velocity);
        weights.push_back(tau);
    } else {
        // compute local UP
        btVector3 up = _currentUp.rotate(axis, -angle);
        btVector3 motorVelocity = motor.velocity;

        // save these non-adjusted components for later
        btVector3 vTargetVelocity = motorVelocity.dot(up) * up;
        btVector3 hTargetVelocity = motorVelocity - vTargetVelocity;

        if (_stepHeight > _minStepHeight && !_steppingUp) {
            // there is a step --> compute velocity direction to go over step
            btVector3 motorVelocityWF = motorVelocity.rotate(axis, angle);
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
        velocity = (hVelocity + vVelocity).rotate(axis, angle);
        _targetVelocity += maxTau * (hTargetVelocity + vTargetVelocity).rotate(axis, angle);

        // store velocity and weights
        velocities.push_back(velocity);
        weights.push_back(maxTau);
    }
}

void CharacterController::computeNewVelocity(btScalar dt, btVector3& velocity) {
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

    // 'thrust' is applied at the very end
    _targetVelocity += dt * _linearAcceleration;
    velocity += dt * _linearAcceleration;
    // Note the differences between these two variables:
    // _targetVelocity = ideal final velocity according to input
    // velocity = real final velocity after motors are applied to current velocity
}

void CharacterController::computeNewVelocity(btScalar dt, glm::vec3& velocity) {
    btVector3 btVelocity = glmToBullet(velocity);
    computeNewVelocity(dt, btVelocity);
    velocity = bulletToGLM(btVelocity);
}

void CharacterController::updateState() {
    if (!_dynamicsWorld) {
        return;
    }
    const btScalar FLY_TO_GROUND_THRESHOLD = 0.1f * _radius;
    const btScalar GROUND_TO_FLY_THRESHOLD = 0.8f * _radius + _halfHeight;
    const quint64 TAKE_OFF_TO_IN_AIR_PERIOD = 250 * MSECS_PER_SECOND;
    const btScalar MIN_HOVER_HEIGHT = _scaleFactor * DEFAULT_AVATAR_MIN_HOVER_HEIGHT;
    const quint64 JUMP_TO_HOVER_PERIOD = 1100 * MSECS_PER_SECOND;

    // scan for distant floor
    // rayStart is at center of bottom sphere
    btVector3 rayStart = _position;

    btScalar rayLength = _radius;
    int16_t collisionGroup = computeCollisionGroup();
    if (collisionGroup == BULLET_COLLISION_GROUP_MY_AVATAR) {
        rayLength += _scaleFactor * DEFAULT_AVATAR_FALL_HEIGHT;
    } else {
        rayLength += MIN_HOVER_HEIGHT;
    }
    btVector3 rayEnd = rayStart - rayLength * _currentUp;

    ClosestNotMe rayCallback(_rigidBody);
    rayCallback.m_closestHitFraction = 1.0f;
    _dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);
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
    if (collisionGroup == BULLET_COLLISION_GROUP_MY_AVATAR) {
        switch (_state) {
        case State::Ground:
            if (!rayHasHit && !_hasSupport) {
                SET_STATE(State::Hover, "no ground detected");
            } else if (_pendingFlags & PENDING_FLAG_JUMP && _jumpButtonDownCount != _takeoffJumpButtonID) {
                _takeoffJumpButtonID = _jumpButtonDownCount;
                _takeoffToInAirStartTime = now;
                SET_STATE(State::Takeoff, "jump pressed");
            } else if (rayHasHit && !_hasSupport && _floorDistance > GROUND_TO_FLY_THRESHOLD) {
                SET_STATE(State::InAir, "falling");
            }
            break;
        case State::Takeoff:
            if (!rayHasHit && !_hasSupport) {
                SET_STATE(State::Hover, "no ground");
            } else if ((now - _takeoffToInAirStartTime) > TAKE_OFF_TO_IN_AIR_PERIOD) {
                SET_STATE(State::InAir, "takeoff done");

                // compute jumpSpeed based on the scaled jump height for the default avatar in default gravity.
                float jumpSpeed = sqrtf(2.0f * DEFAULT_AVATAR_GRAVITY * _scaleFactor * DEFAULT_AVATAR_JUMP_HEIGHT);
                velocity += jumpSpeed * _currentUp;
                _rigidBody->setLinearVelocity(velocity);
            }
            break;
        case State::InAir: {
            const float JUMP_SPEED = _scaleFactor * DEFAULT_AVATAR_JUMP_SPEED;
            if ((velocity.dot(_currentUp) <= (JUMP_SPEED / 2.0f)) && ((_floorDistance < FLY_TO_GROUND_THRESHOLD) || _hasSupport)) {
                SET_STATE(State::Ground, "hit ground");
            } else {
                btVector3 desiredVelocity = _targetVelocity;
                if (desiredVelocity.length2() < MIN_TARGET_SPEED_SQUARED) {
                    desiredVelocity = btVector3(0.0f, 0.0f, 0.0f);
                }
                bool vertTargetSpeedIsNonZero = desiredVelocity.dot(_currentUp) > MIN_TARGET_SPEED;
                if ((jumpButtonHeld || vertTargetSpeedIsNonZero) && (_takeoffJumpButtonID != _jumpButtonDownCount)) {
                    SET_STATE(State::Hover, "double jump button");
                } else if ((jumpButtonHeld || vertTargetSpeedIsNonZero) && (now - _jumpButtonDownStartTime) > JUMP_TO_HOVER_PERIOD) {
                    SET_STATE(State::Hover, "jump button held");
                }
            }
            break;
        }
        case State::Hover:
            btScalar horizontalSpeed = (velocity - velocity.dot(_currentUp) * _currentUp).length();
            bool flyingFast = horizontalSpeed > (MAX_WALKING_SPEED * 0.75f);

            if ((_floorDistance < MIN_HOVER_HEIGHT) && !jumpButtonHeld && !flyingFast) {
                SET_STATE(State::InAir, "near ground");
            } else if (((_floorDistance < FLY_TO_GROUND_THRESHOLD) || _hasSupport) && !flyingFast) {
                SET_STATE(State::Ground, "touching ground");
            }
            break;
        }
    } else {
        // when collisionless: only switch between State::Ground and State::Hover
        // and bypass state debugging
        if (rayHasHit) {
            if (velocity.length() > (MAX_WALKING_SPEED)) {
                _state = State::Hover;
            } else {
                _state = State::Ground;
            }
        } else {
            _state = State::Hover;
        }
    }
}

void CharacterController::preSimulation() {
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

void CharacterController::setFlyingAllowed(bool value) {
    if (value != _flyingAllowed) {
        _flyingAllowed = value;
        if (!_flyingAllowed && _state == State::Hover) {
            SET_STATE(State::InAir, "flying not allowed");
        }
    }
}

void CharacterController::setCollisionlessAllowed(bool value) {
    if (value != _collisionlessAllowed) {
        _collisionlessAllowed = value;
        _pendingFlags |= PENDING_FLAG_UPDATE_COLLISION_GROUP;
    }
}
