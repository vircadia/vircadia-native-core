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

#include <NumericalConstants.h>

#include "PhysicsCollisionGroups.h"
#include "ObjectMotionState.h"

const btVector3 LOCAL_UP_AXIS(0.0f, 1.0f, 0.0f);
const float JUMP_SPEED = 3.5f;
const float MAX_FALL_HEIGHT = 20.0f;

// helper class for simple ray-traces from character
class ClosestNotMe : public btCollisionWorld::ClosestRayResultCallback {
public:
    ClosestNotMe(btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
        _me = me;
        // the RayResultCallback's group and mask must match MY_AVATAR
        m_collisionFilterGroup = BULLET_COLLISION_GROUP_MY_AVATAR;
        m_collisionFilterMask = BULLET_COLLISION_MASK_MY_AVATAR;
    }
    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) {
        if (rayResult.m_collisionObject == _me) {
            return 1.0f;
        }
        return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
protected:
    btRigidBody* _me;
};


CharacterController::CharacterController() {
   _halfHeight = 1.0f;

    _enabled = false;

    _floorDistance = MAX_FALL_HEIGHT;

    _walkVelocity.setValue(0.0f, 0.0f, 0.0f);
    _followDesiredBodyTransform.setIdentity();
    _followTimeRemaining = 0.0f;
    _jumpSpeed = JUMP_SPEED;
    _isOnGround = false;
    _isJumping = false;
    _isFalling = false;
    _isHovering = true;
    _isPushingUp = false;
    _jumpToHoverStart = 0;
    _followTime = 0.0f;
    _followLinearDisplacement = btVector3(0, 0, 0);
    _followAngularDisplacement = btQuaternion::getIdentity();

    _pendingFlags = PENDING_FLAG_UPDATE_SHAPE;

}

bool CharacterController::needsRemoval() const {
    return ((_pendingFlags & PENDING_FLAG_REMOVE_FROM_SIMULATION) == PENDING_FLAG_REMOVE_FROM_SIMULATION);
}

bool CharacterController::needsAddition() const {
    return ((_pendingFlags & PENDING_FLAG_ADD_TO_SIMULATION) == PENDING_FLAG_ADD_TO_SIMULATION);
}

void CharacterController::setDynamicsWorld(btDynamicsWorld* world) {
    if (_dynamicsWorld != world) {
        if (_dynamicsWorld) {
            if (_rigidBody) {
                _dynamicsWorld->removeRigidBody(_rigidBody);
                _dynamicsWorld->removeAction(this);
            }
            _dynamicsWorld = nullptr;
        }
        if (world && _rigidBody) {
            _dynamicsWorld = world;
            _pendingFlags &= ~PENDING_FLAG_JUMP;
            // Before adding the RigidBody to the world we must save its oldGravity to the side
            // because adding an object to the world will overwrite it with the default gravity.
            btVector3 oldGravity = _rigidBody->getGravity();
            _dynamicsWorld->addRigidBody(_rigidBody, BULLET_COLLISION_GROUP_MY_AVATAR, BULLET_COLLISION_MASK_MY_AVATAR);
            _dynamicsWorld->addAction(this);
            // restore gravity settings
            _rigidBody->setGravity(oldGravity);
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
}

void CharacterController::playerStep(btCollisionWorld* dynaWorld, btScalar dt) {
    btVector3 actualVelocity = _rigidBody->getLinearVelocity();
    btScalar actualSpeed = actualVelocity.length();

    btVector3 desiredVelocity = _walkVelocity;
    btScalar desiredSpeed = desiredVelocity.length();

    const btScalar MIN_UP_PUSH = 0.1f;
    if (desiredVelocity.dot(_currentUp) < MIN_UP_PUSH) {
        _isPushingUp = false;
    }

    const btScalar MIN_SPEED = 0.001f;
    if (_isHovering) {
        if (desiredSpeed < MIN_SPEED) {
            if (actualSpeed < MIN_SPEED) {
                _rigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                const btScalar HOVER_BRAKING_TIMESCALE = 0.1f;
                btScalar tau = glm::max(dt / HOVER_BRAKING_TIMESCALE, 1.0f);
                _rigidBody->setLinearVelocity((1.0f - tau) * actualVelocity);
            }
        } else {
            const btScalar HOVER_ACCELERATION_TIMESCALE = 0.1f;
            btScalar tau = dt / HOVER_ACCELERATION_TIMESCALE;
            _rigidBody->setLinearVelocity(actualVelocity - tau * (actualVelocity - desiredVelocity));
        }
    } else {
        if (onGround()) {
            // walking on ground
            if (desiredSpeed < MIN_SPEED) {
                if (actualSpeed < MIN_SPEED) {
                    _rigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
                } else {
                    const btScalar HOVER_BRAKING_TIMESCALE = 0.1f;
                    btScalar tau = dt / HOVER_BRAKING_TIMESCALE;
                    _rigidBody->setLinearVelocity((1.0f - tau) * actualVelocity);
                }
            } else {
                // TODO: modify desiredVelocity using floor normal
                const btScalar WALK_ACCELERATION_TIMESCALE = 0.1f;
                btScalar tau = dt / WALK_ACCELERATION_TIMESCALE;
                btVector3 velocityCorrection = tau * (desiredVelocity - actualVelocity);
                // subtract vertical component
                velocityCorrection -= velocityCorrection.dot(_currentUp) * _currentUp;
                _rigidBody->setLinearVelocity(actualVelocity + velocityCorrection);
            }
        } else {
            // transitioning to flying
            btVector3 velocityCorrection = desiredVelocity - actualVelocity;
            const btScalar FLY_ACCELERATION_TIMESCALE = 0.2f;
            btScalar tau = dt / FLY_ACCELERATION_TIMESCALE;
            if (!_isPushingUp) {
                // actually falling --> compute a different velocity attenuation factor
                const btScalar FALL_ACCELERATION_TIMESCALE = 2.0f;
                tau = dt / FALL_ACCELERATION_TIMESCALE;
                // zero vertical component
                velocityCorrection -= velocityCorrection.dot(_currentUp) * _currentUp;
            }
            _rigidBody->setLinearVelocity(actualVelocity + tau * velocityCorrection);
        }
    }

    // Dynamicaly compute a follow velocity to move this body toward the _followDesiredBodyTransform.
    // Rather then add this velocity to velocity the RigidBody, we explicitly teleport the RigidBody towards its goal.
    // This mirrors the computation done in MyAvatar::FollowHelper::postPhysicsUpdate().
    // These two computations must be kept in sync.
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
        glm::vec2 currentFacing = getFacingDir2D(bulletToGLM(startRot));
        glm::vec2 currentRight(currentFacing.y, -currentFacing.x);
        glm::vec2 desiredFacing = getFacingDir2D(bulletToGLM(_followDesiredBodyTransform.getRotation()));
        float deltaAngle = acosf(glm::clamp(glm::dot(currentFacing, desiredFacing), -1.0f, 1.0f));
        float angularSpeed = deltaAngle / _followTimeRemaining;
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
}

void CharacterController::jump() {
    // check for case where user is holding down "jump" key...
    // we'll eventually tansition to "hover"
    if (!_isJumping) {
        if (!_isHovering) {
            _jumpToHoverStart = usecTimestampNow();
            _pendingFlags |= PENDING_FLAG_JUMP;
        }
    } else {
        quint64 now = usecTimestampNow();
        const quint64 JUMP_TO_HOVER_PERIOD = 75 * (USECS_PER_SECOND / 100);
        if (now - _jumpToHoverStart > JUMP_TO_HOVER_PERIOD) {
            _isPushingUp = true;
            setHovering(true);
        }
    }
}

bool CharacterController::onGround() const {
    const btScalar FLOOR_PROXIMITY_THRESHOLD = 0.3f * _radius;
    return _floorDistance < FLOOR_PROXIMITY_THRESHOLD;
}

void CharacterController::setHovering(bool hover) {
    if (hover != _isHovering) {
        _isHovering = hover;
        _isJumping = false;

        if (_rigidBody) {
            if (hover) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(DEFAULT_CHARACTER_GRAVITY * _currentUp);
            }
        }
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
        // only need to ADD back when we happen to be enabled
        if (_enabled) {
            _pendingFlags |= PENDING_FLAG_ADD_TO_SIMULATION;
        }
    }

    // it's ok to change offset immediately -- there are no thread safety issues here
    _shapeLocalOffset = corner + 0.5f * _boxScale;
}

void CharacterController::setEnabled(bool enabled) {
    if (enabled != _enabled) {
        if (enabled) {
            // Don't bother clearing REMOVE bit since it might be paired with an UPDATE_SHAPE bit.
            // Setting the ADD bit here works for all cases so we don't even bother checking other bits.
            _pendingFlags |= PENDING_FLAG_ADD_TO_SIMULATION;
        } else {
            if (_dynamicsWorld) {
                _pendingFlags |= PENDING_FLAG_REMOVE_FROM_SIMULATION;
            }
            _pendingFlags &= ~ PENDING_FLAG_ADD_TO_SIMULATION;
            _isOnGround = false;
        }
        setHovering(true);
        _enabled = enabled;
    }
}

void CharacterController::updateUpAxis(const glm::quat& rotation) {
    btVector3 oldUp = _currentUp;
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    if (!_isHovering) {
        const btScalar MIN_UP_ERROR = 0.01f;
        if (oldUp.distance(_currentUp) > MIN_UP_ERROR) {
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
    if (_enabled && _rigidBody) {
        const btTransform& avatarTransform = _rigidBody->getWorldTransform();
        rotation = bulletToGLM(avatarTransform.getRotation());
        position = bulletToGLM(avatarTransform.getOrigin()) - rotation * _shapeLocalOffset;
    }
}

void CharacterController::setTargetVelocity(const glm::vec3& velocity) {
    //_walkVelocity = glmToBullet(_avatarData->getTargetVelocity());
    _walkVelocity = glmToBullet(velocity);
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

glm::vec3 CharacterController::getLinearVelocity() const {
    glm::vec3 velocity(0.0f);
    if (_rigidBody) {
        velocity = bulletToGLM(_rigidBody->getLinearVelocity());
    }
    return velocity;
}

void CharacterController::preSimulation() {
    if (_enabled && _dynamicsWorld) {
        // slam body to where it is supposed to be
        _rigidBody->setWorldTransform(_characterBodyTransform);

        // scan for distant floor
        // rayStart is at center of bottom sphere
        btVector3 rayStart = _characterBodyTransform.getOrigin() - _halfHeight * _currentUp;

        // rayEnd is straight down MAX_FALL_HEIGHT
        btScalar rayLength = _radius + MAX_FALL_HEIGHT;
        btVector3 rayEnd = rayStart - rayLength * _currentUp;

        ClosestNotMe rayCallback(_rigidBody);
        rayCallback.m_closestHitFraction = 1.0f;
        _dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);
        if (rayCallback.hasHit()) {
            _floorDistance = rayLength * rayCallback.m_closestHitFraction - _radius;
            const btScalar MIN_HOVER_HEIGHT = 3.0f;
            if (_isHovering && _floorDistance < MIN_HOVER_HEIGHT && !_isPushingUp) {
                setHovering(false);
            }
            // TODO: use collision events rather than ray-trace test to disable jumping
            const btScalar JUMP_PROXIMITY_THRESHOLD = 0.1f * _radius;
            if (_floorDistance < JUMP_PROXIMITY_THRESHOLD) {
                _isJumping = false;
            }
        } else {
            _floorDistance = FLT_MAX;
            setHovering(true);
        }

        if (_pendingFlags & PENDING_FLAG_JUMP) {
            _pendingFlags &= ~ PENDING_FLAG_JUMP;
            if (onGround()) {
                _isJumping = true;
                btVector3 velocity = _rigidBody->getLinearVelocity();
                velocity += _jumpSpeed * _currentUp;
                _rigidBody->setLinearVelocity(velocity);
            }
        }
    }
    _followTime = 0.0f;

    _followLinearDisplacement = btVector3(0, 0, 0);
    _followAngularDisplacement = btQuaternion::getIdentity();
}

void CharacterController::postSimulation() {
    // postSimulation() exists for symmetry and just in case we need to do something here later
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
