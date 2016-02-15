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

    _targetVelocity.setValue(0.0f, 0.0f, 0.0f);
    _followDesiredBodyTransform.setIdentity();
    _followTimeRemaining = 0.0f;
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

void CharacterController::playerStep(btCollisionWorld* dynaWorld, btScalar dt) {

    const btScalar MIN_SPEED = 0.001f;

    btVector3 actualVelocity = _rigidBody->getLinearVelocity() - _parentVelocity;
    if (actualVelocity.length() < MIN_SPEED) {
        actualVelocity = btVector3(0.0f, 0.0f, 0.0f);
    }

    btVector3 desiredVelocity = _targetVelocity;
    if (desiredVelocity.length() < MIN_SPEED) {
        desiredVelocity = btVector3(0.0f, 0.0f, 0.0f);
    }

    // decompose into horizontal and vertical components.
    btVector3 actualVertVelocity = actualVelocity.dot(_currentUp) * _currentUp;
    btVector3 actualHorizVelocity = actualVelocity - actualVertVelocity;
    btVector3 desiredVertVelocity = desiredVelocity.dot(_currentUp) * _currentUp;
    btVector3 desiredHorizVelocity = desiredVelocity - desiredVertVelocity;

    btVector3 finalVelocity;

    switch (_state) {
    case State::Ground:
    case State::Takeoff:
        {
            // horizontal ground control
            const btScalar WALK_ACCELERATION_TIMESCALE = 0.1f;
            btScalar tau = dt / WALK_ACCELERATION_TIMESCALE;
            finalVelocity = tau * desiredHorizVelocity + (1.0f - tau) * actualHorizVelocity + actualVertVelocity;
        }
        break;
    case State::InAir:
        {
            // horizontal air control
            const btScalar IN_AIR_ACCELERATION_TIMESCALE = 2.0f;
            btScalar tau = dt / IN_AIR_ACCELERATION_TIMESCALE;
            finalVelocity = tau * desiredHorizVelocity + (1.0f - tau) * actualHorizVelocity + actualVertVelocity;
        }
        break;
    case State::Hover:
        {
            // vertical and horizontal air control
            const btScalar FLY_ACCELERATION_TIMESCALE = 0.2f;
            btScalar tau = dt / FLY_ACCELERATION_TIMESCALE;
            finalVelocity = tau * desiredVelocity + (1.0f - tau) * actualVelocity;
        }
        break;
    }

    _rigidBody->setLinearVelocity(finalVelocity + _parentVelocity);

    // Dynamicaly compute a follow velocity to move this body toward the _followDesiredBodyTransform.
    // Rather then add this velocity to velocity the RigidBody, we explicitly teleport the RigidBody towards its goal.
    // This mirrors the computation done in MyAvatar::FollowHelper::postPhysicsUpdate().

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
    if (desiredState != _state) {
#ifdef DEBUG_STATE_CHANGE
        qCDebug(physics) << "CharacterController::setState" << stateToStr(desiredState) << "from" << stateToStr(_state) << "," << reason;
#endif
        if (desiredState == State::Hover && _state != State::Hover) {
            // hover enter
            if (_rigidBody) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            }
        } else if (_state == State::Hover && desiredState != State::Hover) {
            // hover exit
            if (_rigidBody) {
                _rigidBody->setGravity(DEFAULT_CHARACTER_GRAVITY * _currentUp);
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
        }
        SET_STATE(State::Hover, "setEnabled");
        _enabled = enabled;
    }
}

void CharacterController::updateUpAxis(const glm::quat& rotation) {
    btVector3 oldUp = _currentUp;
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    if (_state != State::Hover) {
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
    _targetVelocity = glmToBullet(velocity);
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

void CharacterController::preSimulation() {
    if (_enabled && _dynamicsWorld) {
        quint64 now = usecTimestampNow();

        // slam body to where it is supposed to be
        _rigidBody->setWorldTransform(_characterBodyTransform);
        btVector3 velocity = _rigidBody->getLinearVelocity();

        btVector3 actualVertVelocity = velocity.dot(_currentUp) * _currentUp;
        btVector3 actualHorizVelocity = velocity - actualVertVelocity;

        // scan for distant floor
        // rayStart is at center of bottom sphere
        btVector3 rayStart = _characterBodyTransform.getOrigin() - _halfHeight * _currentUp;

        // rayEnd is straight down MAX_FALL_HEIGHT
        btScalar rayLength = _radius + MAX_FALL_HEIGHT;
        btVector3 rayEnd = rayStart - rayLength * _currentUp;

        const btScalar JUMP_PROXIMITY_THRESHOLD = 0.1f * _radius;
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
            _floorDistance = rayLength * rayCallback.m_closestHitFraction - _radius;
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
        bool flyingFast = _state == State::Hover && actualHorizVelocity.length() > (MAX_WALKING_SPEED * 0.75f);

        switch (_state) {
        case State::Ground:
            if (!rayHasHit && !_hasSupport) {
                SET_STATE(State::Hover, "no ground detected");
            } else if (_pendingFlags & PENDING_FLAG_JUMP && _jumpButtonDownCount != _takeoffJumpButtonID) {
                _takeoffJumpButtonID = _jumpButtonDownCount;
                _takeoffToInAirStartTime = now;
                SET_STATE(State::Takeoff, "jump pressed");
            } else if (rayHasHit && !_hasSupport && _floorDistance > JUMP_PROXIMITY_THRESHOLD) {
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
            if ((velocity.dot(_currentUp) <= (JUMP_SPEED / 2.0f)) && ((_floorDistance < JUMP_PROXIMITY_THRESHOLD) || _hasSupport)) {
                SET_STATE(State::Ground, "hit ground");
            } else if (jumpButtonHeld && (_takeoffJumpButtonID != _jumpButtonDownCount)) {
                SET_STATE(State::Hover, "double jump button");
            } else if (jumpButtonHeld && (now - _jumpButtonDownStartTime) > JUMP_TO_HOVER_PERIOD) {
                SET_STATE(State::Hover, "jump button held");
            }
            break;
        }
        case State::Hover:
            if ((_floorDistance < MIN_HOVER_HEIGHT) && !jumpButtonHeld && !flyingFast) {
                SET_STATE(State::InAir, "near ground");
            } else if (((_floorDistance < JUMP_PROXIMITY_THRESHOLD) || _hasSupport) && !flyingFast) {
                SET_STATE(State::Ground, "touching ground");
            }
            break;
        }
    }

    _previousFlags = _pendingFlags;
    _pendingFlags &= ~PENDING_FLAG_JUMP;

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
