//
//  CharacterControllerInterface.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CharacterController.h"

//#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>

#include <NumericalConstants.h>

#include "ObjectMotionState.h"
#include "PhysicsLogging.h"

const btVector3 LOCAL_UP_AXIS(0.0f, 1.0f, 0.0f);
const float JUMP_SPEED = 3.5f;
const float MAX_FALL_HEIGHT = 20.0f;

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
    _halfHeight = 1.0f;
    _floorDistance = MAX_FALL_HEIGHT;

    _targetVelocity.setValue(0.0f, 0.0f, 0.0f);
    _followDesiredBodyTransform.setIdentity();
    _jumpSpeed = JUMP_SPEED;
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
        if (world && _rigidBody) {
            // add to new world
            _dynamicsWorld = world;
            _pendingFlags &= ~PENDING_FLAG_JUMP;
            // Before adding the RigidBody to the world we must save its oldGravity to the side
            // because adding an object to the world will overwrite it with the default gravity.
            btVector3 oldGravity = _rigidBody->getGravity();
            _dynamicsWorld->addRigidBody(_rigidBody, _collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR);
            _dynamicsWorld->addAction(this);
            // restore gravity settings
            _rigidBody->setGravity(oldGravity);
            btCollisionShape* shape = _rigidBody->getCollisionShape();
            assert(shape && shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE);
            _ghost.setCharacterShape(static_cast<btCapsuleShape*>(shape)); // KINEMATIC_CONTROLLER_HACK
        }
        // KINEMATIC_CONTROLLER_HACK
        _ghost.setCollisionGroupAndMask(_collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR & (~ _collisionGroup));
        _ghost.setCollisionWorld(_dynamicsWorld);
        _ghost.setDistanceToFeet(_radius + _halfHeight);
        _ghost.setMaxStepHeight(0.75f * (_radius + _halfHeight)); // HACK
        _ghost.setMinWallAngle(PI / 4.0f); // HACK
        _ghost.setUpDirection(_currentUp);
        _ghost.setGravity(DEFAULT_CHARACTER_GRAVITY);
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

static const float COS_PI_OVER_THREE = cosf(PI / 3.0f);

bool CharacterController::checkForSupport(btCollisionWorld* collisionWorld) const {
    int numManifolds = collisionWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = collisionWorld->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
        const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
        if (obA == _rigidBody || obB == _rigidBody) {
            int numContacts = contactManifold->getNumContacts();
            for (int j = 0; j < numContacts; j++) {
                btManifoldPoint& pt = contactManifold->getContactPoint(j);

                // check to see if contact point is touching the bottom sphere of the capsule.
                // and the contact normal is not slanted too much.
                float contactPointY = (obA == _rigidBody) ? pt.m_localPointA.getY() : pt.m_localPointB.getY();
                btVector3 normal = (obA == _rigidBody) ? pt.m_normalWorldOnB : -pt.m_normalWorldOnB;
                if (contactPointY < -_halfHeight && normal.dot(_currentUp) > COS_PI_OVER_THREE) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CharacterController::preStep(btCollisionWorld* collisionWorld) {
    // trace a ray straight down to see if we're standing on the ground
    const btTransform& xform = _rigidBody->getWorldTransform();

    // rayStart is at center of bottom sphere
    btVector3 rayStart = xform.getOrigin() - _halfHeight * _currentUp;

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

    _hasSupport = checkForSupport(collisionWorld);
}

const btScalar MIN_TARGET_SPEED = 0.001f;
const btScalar MIN_TARGET_SPEED_SQUARED = MIN_TARGET_SPEED * MIN_TARGET_SPEED;

void CharacterController::playerStep(btCollisionWorld* dynaWorld, btScalar dt) {
    btVector3 velocity = _rigidBody->getLinearVelocity() - _parentVelocity;
    computeNewVelocity(dt, velocity);

    if (_moveKinematically) {
        // KINEMATIC_CONTROLLER_HACK
        btTransform transform = _rigidBody->getWorldTransform();
        transform.setOrigin(_ghost.getWorldTransform().getOrigin());
        _ghost.setWorldTransform(transform);
        _ghost.setMotorVelocity(_simpleMotorVelocity);
        float overshoot = 1.0f * _radius;
        _ghost.move(dt, overshoot);
        _rigidBody->setWorldTransform(_ghost.getWorldTransform());
        _rigidBody->setLinearVelocity(_ghost.getLinearVelocity());
    } else {
        // Dynamicaly compute a follow velocity to move this body toward the _followDesiredBodyTransform.
        // Rather than add this velocity to velocity the RigidBody, we explicitly teleport the RigidBody towards its goal.
        // This mirrors the computation done in MyAvatar::FollowHelper::postPhysicsUpdate().

        _rigidBody->setLinearVelocity(velocity + _parentVelocity);
        if (_following) {
            // OUTOFBODY_HACK -- these consts were copied from elsewhere, and then tuned
            const float NORMAL_WALKING_SPEED = 0.5f;
            const float FOLLOW_TIME = 0.8f;
            const float FOLLOW_ROTATION_THRESHOLD = cosf(PI / 6.0f);

            const float MAX_ANGULAR_SPEED = FOLLOW_ROTATION_THRESHOLD / FOLLOW_TIME;

            btTransform bodyTransform = _rigidBody->getWorldTransform();

            btVector3 startPos = bodyTransform.getOrigin();
            btVector3 deltaPos = _followDesiredBodyTransform.getOrigin() - startPos;
            btVector3 vel = deltaPos * (0.5f / dt);
            btScalar speed = vel.length();
            if (speed > NORMAL_WALKING_SPEED) {
                vel *= NORMAL_WALKING_SPEED / speed;
            }
            btVector3 linearDisplacement = vel * dt;
            btVector3 endPos = startPos + linearDisplacement;

            btQuaternion startRot = bodyTransform.getRotation();
            glm::vec2 currentFacing = getFacingDir2D(bulletToGLM(startRot));
            glm::vec2 currentRight(currentFacing.y, -currentFacing.x);
            glm::vec2 desiredFacing = getFacingDir2D(bulletToGLM(_followDesiredBodyTransform.getRotation()));
            float deltaAngle = acosf(glm::clamp(glm::dot(currentFacing, desiredFacing), -1.0f, 1.0f));
            float angularSpeed = 0.5f * deltaAngle / dt;
            if (angularSpeed > MAX_ANGULAR_SPEED) {
                angularSpeed *= MAX_ANGULAR_SPEED / angularSpeed;
            }
            float sign = copysignf(1.0f, glm::dot(desiredFacing, currentRight));
            btQuaternion angularDisplacement = btQuaternion(btVector3(0.0f, 1.0f, 0.0f), sign * angularSpeed * dt);
            btQuaternion endRot = angularDisplacement * startRot;

            // in order to accumulate displacement of avatar position, we need to take _shapeLocalOffset into account.
            btVector3 shapeLocalOffset = glmToBullet(_shapeLocalOffset);
            btVector3 swingDisplacement = rotateVector(endRot, -shapeLocalOffset) - rotateVector(startRot, -shapeLocalOffset);

            _followLinearDisplacement = linearDisplacement + swingDisplacement + _followLinearDisplacement;
            _followAngularDisplacement = angularDisplacement * _followAngularDisplacement;

            _rigidBody->setWorldTransform(btTransform(endRot, endPos));
        }
        _followTime += dt;
        _ghost.setWorldTransform(_rigidBody->getWorldTransform());
    }
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
        if (_rigidBody) {
            if (desiredState == State::Hover && _state != State::Hover) {
                // hover enter
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else if (_state == State::Hover && desiredState != State::Hover) {
                // hover exit
                if (_collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS) {
                    _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
                } else {
                    _rigidBody->setGravity(DEFAULT_CHARACTER_GRAVITY * _currentUp);
                }
            }
        }
        _state = desiredState;
    }
}

void CharacterController::setLocalBoundingBox(const glm::vec3& corner, const glm::vec3& scale) {
    _boxScale = scale;

    float x = _boxScale.x;
    float z = _boxScale.z;
    float radius = 0.5f * sqrtf(0.5f * (x * x + z * z));
    float halfHeight = 0.5f * _boxScale.y - radius;
    float MIN_HALF_HEIGHT = 0.1f;
    if (halfHeight < MIN_HALF_HEIGHT) {
        halfHeight = MIN_HALF_HEIGHT;
    }

    // compare dimensions
    float radiusDelta = glm::abs(radius - _radius);
    float heightDelta = glm::abs(halfHeight - _halfHeight);
    if (radiusDelta < FLT_EPSILON && heightDelta < FLT_EPSILON) {
        // shape hasn't changed --> nothing to do
    } else {
        if (_dynamicsWorld) {
            // must REMOVE from world prior to shape update
            _pendingFlags |= PENDING_FLAG_REMOVE_FROM_SIMULATION;
        }
        _pendingFlags |= PENDING_FLAG_UPDATE_SHAPE;
        _pendingFlags |= PENDING_FLAG_ADD_TO_SIMULATION;
    }

    // it's ok to change offset immediately -- there are no thread safety issues here
    _shapeLocalOffset = corner + 0.5f * _boxScale;
}

void CharacterController::setCollisionGroup(int16_t group) {
    if (_collisionGroup != group) {
        _collisionGroup = group;
        _pendingFlags |= PENDING_FLAG_UPDATE_COLLISION_GROUP;
        _ghost.setCollisionGroupAndMask(_collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR & (~ _collisionGroup));
    }
}

void CharacterController::handleChangedCollisionGroup() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_COLLISION_GROUP) {
        // ATM the easiest way to update collision groups is to remove/re-add the RigidBody
        if (_dynamicsWorld) {
            _dynamicsWorld->removeRigidBody(_rigidBody);
            _dynamicsWorld->addRigidBody(_rigidBody, _collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR);
        }
        _pendingFlags &= ~PENDING_FLAG_UPDATE_COLLISION_GROUP;

        if (_state != State::Hover && _rigidBody) {
            if (_collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(DEFAULT_CHARACTER_GRAVITY * _currentUp);
            }
        }
    }
}

void CharacterController::updateUpAxis(const glm::quat& rotation) {
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    _ghost.setUpDirection(_currentUp);
    if (_state != State::Hover && _rigidBody) {
        if (_collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS) {
            _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
        } else {
            _rigidBody->setGravity(DEFAULT_CHARACTER_GRAVITY * _currentUp);
        }
    }
}

void CharacterController::setPositionAndOrientation(
        const glm::vec3& position,
        const glm::quat& orientation) {
    // TODO: update gravity if up has changed
    updateUpAxis(orientation);

    btQuaternion bodyOrientation = glmToBullet(orientation);
    btVector3 bodyPosition = glmToBullet(position + orientation * _shapeLocalOffset);
    _characterBodyTransform = btTransform(bodyOrientation, bodyPosition);
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
    _following = true;
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

void CharacterController::setCapsuleRadius(float radius) {
    _radius = radius;
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

    if (_collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS ||
            _state == State::Hover || motor.hTimescale == motor.vTimescale) {
        // modify velocity
        btScalar tau = dt / motor.hTimescale;
        if (tau > 1.0f) {
            tau = 1.0f;
        }
        velocity += (motor.velocity - velocity) * tau;

        // rotate back into world-frame
        velocity = velocity.rotate(axis, angle);

        // store the velocity and weight
        velocities.push_back(velocity);
        weights.push_back(tau);
    } else {
        // compute local UP
        btVector3 up = _currentUp.rotate(axis, -angle);

        // split velocity into horizontal and vertical components
        btVector3 vVelocity = velocity.dot(up) * up;
        btVector3 hVelocity = velocity - vVelocity;
        btVector3 vTargetVelocity = motor.velocity.dot(up) * up;
        btVector3 hTargetVelocity = motor.velocity - vTargetVelocity;

        // modify each component separately
        btScalar maxTau = 0.0f;
        if (motor.hTimescale < MAX_CHARACTER_MOTOR_TIMESCALE) {
            btScalar tau = dt / motor.hTimescale;
            if (tau > 1.0f) {
                tau = 1.0f;
            }
            maxTau = tau;
            hVelocity += (hTargetVelocity - hVelocity) * tau;
        }
        if (motor.vTimescale < MAX_CHARACTER_MOTOR_TIMESCALE) {
            btScalar tau = dt / motor.vTimescale;
            if (tau > 1.0f) {
                tau = 1.0f;
            }
            if (tau > maxTau) {
                maxTau = tau;
            }
            vVelocity += (vTargetVelocity - vVelocity) * tau;
        }

        // add components back together and rotate into world-frame
        velocity = (hVelocity + vVelocity).rotate(axis, angle);
        _simpleMotorVelocity += maxTau * (hTargetVelocity + vTargetVelocity).rotate(axis, angle);

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
    _simpleMotorVelocity = btVector3(0.0f, 0.0f, 0.0f);
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
        _simpleMotorVelocity /= totalWeight;
    }
    if (velocity.length2() < MIN_TARGET_SPEED_SQUARED) {
        velocity = btVector3(0.0f, 0.0f, 0.0f);
    }

    // 'thrust' is applied at the very end
    velocity += dt * _linearAcceleration;
    _targetVelocity = velocity;
}

void CharacterController::computeNewVelocity(btScalar dt, glm::vec3& velocity) {
    btVector3 btVelocity = glmToBullet(velocity);
    computeNewVelocity(dt, btVelocity);
    velocity = bulletToGLM(btVelocity);
}

void CharacterController::preSimulation() {
    if (_dynamicsWorld) {
        quint64 now = usecTimestampNow();

        // slam body to where it is supposed to be
        _rigidBody->setWorldTransform(_characterBodyTransform);
        btVector3 velocity = _rigidBody->getLinearVelocity();
        _preSimulationVelocity = velocity;

        // scan for distant floor
        // rayStart is at center of bottom sphere
        btVector3 rayStart = _characterBodyTransform.getOrigin();

        // rayEnd is straight down MAX_FALL_HEIGHT
        btScalar rayLength = _radius + MAX_FALL_HEIGHT;
        btVector3 rayEnd = rayStart - rayLength * _currentUp;

        const btScalar FLY_TO_GROUND_THRESHOLD = 0.1f * _radius;
        const btScalar GROUND_TO_FLY_THRESHOLD = 0.8f * _radius + _halfHeight;
        const quint64 TAKE_OFF_TO_IN_AIR_PERIOD = 250 * MSECS_PER_SECOND;
        const btScalar MIN_HOVER_HEIGHT = 2.5f;
        const quint64 JUMP_TO_HOVER_PERIOD = 1100 * MSECS_PER_SECOND;
        const btScalar MAX_WALKING_SPEED = 2.5f;
        const quint64 RAY_HIT_START_PERIOD = 500 * MSECS_PER_SECOND;

        ClosestNotMe rayCallback(_rigidBody);
        rayCallback.m_closestHitFraction = 1.0f;
        _dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);
        bool rayHasHit = rayCallback.hasHit();
        if (rayHasHit) {
            _rayHitStartTime = now;
            _floorDistance = rayLength * rayCallback.m_closestHitFraction - (_radius + _halfHeight);
        } else if ((now - _rayHitStartTime) < RAY_HIT_START_PERIOD) {
            rayHasHit = true;
        } else {
            _floorDistance = FLT_MAX;
        }

        // record a time stamp when the jump button was first pressed.
        if ((_previousFlags & PENDING_FLAG_JUMP) != (_pendingFlags & PENDING_FLAG_JUMP)) {
            if (_pendingFlags & PENDING_FLAG_JUMP) {
                _jumpButtonDownStartTime = now;
                _jumpButtonDownCount++;
            }
        }

        bool jumpButtonHeld = _pendingFlags & PENDING_FLAG_JUMP;

        btVector3 actualHorizVelocity = velocity - velocity.dot(_currentUp) * _currentUp;
        bool flyingFast = _state == State::Hover && actualHorizVelocity.length() > (MAX_WALKING_SPEED * 0.75f);

        // OUTOFBODY_HACK -- disable normal state transitions while collisionless
        if (_collisionGroup == BULLET_COLLISION_GROUP_MY_AVATAR) {
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
                    velocity += _jumpSpeed * _currentUp;
                    _rigidBody->setLinearVelocity(velocity);
                }
                break;
            case State::InAir: {
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
                if ((_floorDistance < MIN_HOVER_HEIGHT) && !jumpButtonHeld && !flyingFast) {
                    SET_STATE(State::InAir, "near ground");
                } else if (((_floorDistance < FLY_TO_GROUND_THRESHOLD) || _hasSupport) && !flyingFast) {
                    SET_STATE(State::Ground, "touching ground");
                }
                break;
            }
        } else {
            // OUTOFBODY_HACK -- in collisionless state switch between Ground and Hover states
            if (rayHasHit) {
                SET_STATE(State::Ground, "collisionless above ground");
            } else {
                SET_STATE(State::Hover, "collisionless in air");
            }
        }
    }

    _previousFlags = _pendingFlags;
    _pendingFlags &= ~PENDING_FLAG_JUMP;

    _followTime = 0.0f;
    _followLinearDisplacement = btVector3(0.0f, 0.0f, 0.0f);
    _followAngularDisplacement = btQuaternion::getIdentity();
}

void CharacterController::postSimulation() {
    // postSimulation() exists for symmetry and just in case we need to do something here later

    btVector3 velocity = _rigidBody->getLinearVelocity();
    _velocityChange = velocity - _preSimulationVelocity;
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
    if (_flyingAllowed != value) {
        _flyingAllowed = value;

        if (!_flyingAllowed && _state == State::Hover) {
            // OUTOFBODY_HACK -- disable normal state transitions while collisionless
            if (_collisionGroup == BULLET_COLLISION_GROUP_MY_AVATAR) {
                SET_STATE(State::InAir, "flying not allowed");
            }
        }
    }
}

void CharacterController::setMoveKinematically(bool kinematic) {
    if (kinematic != _moveKinematically) {
        _moveKinematically = kinematic;
        _pendingFlags |= PENDING_FLAG_UPDATE_SHAPE;
    }
}
