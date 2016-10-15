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

#include "ObjectMotionState.h"
#include "PhysicsHelpers.h"
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
            _dynamicsWorld->addRigidBody(_rigidBody, _collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR);
            _dynamicsWorld->addAction(this);
            // restore gravity settings because adding an object to the world overwrites its gravity setting
            _rigidBody->setGravity(_gravity * _currentUp);
            btCollisionShape* shape = _rigidBody->getCollisionShape();
            assert(shape && shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE);
            _ghost.setCharacterCapsule(static_cast<btCapsuleShape*>(shape)); // KINEMATIC_CONTROLLER_HACK
        }
        // KINEMATIC_CONTROLLER_HACK
        _ghost.setCollisionGroupAndMask(_collisionGroup, BULLET_COLLISION_MASK_MY_AVATAR & (~ _collisionGroup));
        _ghost.setCollisionWorld(_dynamicsWorld);
        _ghost.setRadiusAndHalfHeight(_radius, _halfHeight);
        _ghost.setMaxStepHeight(0.75f * (_radius + _halfHeight)); // HACK
        _ghost.setMinWallAngle(PI / 4.0f); // HACK
        _ghost.setUpDirection(_currentUp);
        _ghost.setMotorOnly(!_moveKinematically);
        _ghost.setWorldTransform(_rigidBody->getWorldTransform());
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

bool CharacterController::checkForSupport(btCollisionWorld* collisionWorld, btScalar dt) {
    if (_moveKinematically) {
        // kinematic motion will move() the _ghost later
        return _ghost.hasSupport();
    }

    bool pushing = _targetVelocity.length2() > FLT_EPSILON;

    btDispatcher* dispatcher = collisionWorld->getDispatcher();
    int numManifolds = dispatcher->getNumManifolds();
    bool hasFloor = false;
    const float COS_PI_OVER_THREE = cosf(PI / 3.0f);

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
                if (hitHeight < _maxStepHeight && normal.dot(_currentUp) > COS_PI_OVER_THREE) {
                    //std::cout << "adebug manifoldIndex = " << i << "  contactIndex = " << j << "  hitOnCharacter*up = " << pointOnCharacter.dot(_currentUp) << std::endl;  // adebug
                    hasFloor = true;
                    if (!pushing) {
                        // we're not pushing against anything so we can early exit
                        // (all we need to know is that there is a floor)
                        break;
                    }
                }
                if (pushing && _targetVelocity.dot(normal) < 0.0f) {
                    // remember highest step obstacle
                    if (hitHeight > _maxStepHeight) {
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
            if (hasFloor && !pushing) {
                // early exit since all we need to know is that we're on a floor
                break;
            }
        }
    }
    return hasFloor;
}

void CharacterController::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) {
    _preActionVelocity = getLinearVelocity();
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
    btVector3 velocity = _rigidBody->getLinearVelocity() - _parentVelocity;
    if (_following) {
        _followTimeAccumulator += dt;
        // linear part uses a motor
        const float MAX_WALKING_SPEED = 30.5f; // TODO: scale this stuff with avatar size
        const float MAX_WALKING_SPEED_DISTANCE = 1.0f;
        const float NORMAL_WALKING_SPEED = 0.5f * MAX_WALKING_SPEED;
        const float NORMAL_WALKING_SPEED_DISTANCE = 0.5f * MAX_WALKING_SPEED_DISTANCE;
        const float FEW_SUBSTEPS = 4.0f * dt;
        btTransform bodyTransform = _rigidBody->getWorldTransform();
        btVector3 startPos = bodyTransform.getOrigin();
        btVector3 deltaPos = _followDesiredBodyTransform.getOrigin() - startPos;
        btScalar deltaDistance = deltaPos.length();
        const float MIN_DELTA_DISTANCE = 0.01f; // TODO: scale by avatar size but cap at (NORMAL_WALKING_SPEED * FEW_SUBSTEPS)
        if (deltaDistance > MIN_DELTA_DISTANCE) {
            btVector3 vel = deltaPos;
            if (_state == State::Hover) {
                btScalar HOVER_FOLLOW_VELOCITY_TIMESCALE = 0.1f;
                vel /= HOVER_FOLLOW_VELOCITY_TIMESCALE;
            } else {
                if (deltaDistance > MAX_WALKING_SPEED_DISTANCE) {
                    // cap max speed
                    vel *= MAX_WALKING_SPEED / deltaDistance;
                } else if (deltaDistance > NORMAL_WALKING_SPEED_DISTANCE) {
                    // linearly interpolate to NORMAL_WALKING_SPEED
                    btScalar alpha = (deltaDistance - NORMAL_WALKING_SPEED_DISTANCE) / (MAX_WALKING_SPEED_DISTANCE - NORMAL_WALKING_SPEED_DISTANCE);
                    vel *= NORMAL_WALKING_SPEED * (1.0f - alpha) + MAX_WALKING_SPEED * alpha;
                } else {
                    // use exponential decay but cap at NORMAL_WALKING_SPEED
                    vel /= FEW_SUBSTEPS;
                    btScalar speed = vel.length();
                    if (speed > NORMAL_WALKING_SPEED) {
                        vel *= NORMAL_WALKING_SPEED / speed;
                    }
                }
            }
            vel += _followVelocity;
            const float HORIZONTAL_FOLLOW_TIMESCALE = 0.05f;
            const float VERTICAL_FOLLOW_TIMESCALE = (_state == State::Hover) ? HORIZONTAL_FOLLOW_TIMESCALE : 20.0f;
            glm::quat worldFrameRotation; // identity
            vel.setY(0.0f);  // don't allow any vertical component of the follow velocity to enter the _targetVelocity.
            addMotor(bulletToGLM(vel), worldFrameRotation, HORIZONTAL_FOLLOW_TIMESCALE, VERTICAL_FOLLOW_TIMESCALE);
        }

        // angular part uses incremental teleports
        const float ANGULAR_FOLLOW_TIMESCALE = 0.8f;
        const float MAX_ANGULAR_SPEED = (PI / 2.0f) / ANGULAR_FOLLOW_TIMESCALE;
        btQuaternion startRot = bodyTransform.getRotation();
        glm::vec2 currentFacing = getFacingDir2D(bulletToGLM(startRot));
        glm::vec2 currentRight(currentFacing.y, - currentFacing.x);
        glm::vec2 desiredFacing = getFacingDir2D(bulletToGLM(_followDesiredBodyTransform.getRotation()));
        float deltaAngle = acosf(glm::clamp(glm::dot(currentFacing, desiredFacing), -1.0f, 1.0f));
        float angularSpeed = deltaAngle / FEW_SUBSTEPS;
        if (angularSpeed > MAX_ANGULAR_SPEED) {
            angularSpeed *= MAX_ANGULAR_SPEED / angularSpeed;
        }
        float sign = copysignf(1.0f, glm::dot(desiredFacing, currentRight));
        btQuaternion angularDisplacement = btQuaternion(btVector3(0.0f, 1.0f, 0.0f), sign * angularSpeed * dt);
        btQuaternion endRot = angularDisplacement * startRot;
        _rigidBody->setWorldTransform(btTransform(endRot, startPos));
    }

    _hasSupport = checkForSupport(collisionWorld, dt);
    computeNewVelocity(dt, velocity);

    if (_moveKinematically) {
        // KINEMATIC_CONTROLLER_HACK
        btTransform transform = _rigidBody->getWorldTransform();
        transform.setOrigin(_ghost.getWorldTransform().getOrigin());
        _ghost.setWorldTransform(transform);
        _ghost.setMotorVelocity(_targetVelocity);
        float overshoot = 1.0f * _radius;
        _ghost.move(dt, overshoot, _gravity);
        transform.setOrigin(_ghost.getWorldTransform().getOrigin());
        _rigidBody->setWorldTransform(transform);
        _rigidBody->setLinearVelocity(_ghost.getLinearVelocity());
    } else {
        float stepUpSpeed2 = _stepUpVelocity.length2();
        if (stepUpSpeed2 > FLT_EPSILON) {
            // we step up with teleports rather than applying velocity
            // use a speed that would ballistically reach _stepHeight under gravity
            _stepUpVelocity /= sqrtf(stepUpSpeed2);
            btScalar minStepUpSpeed = sqrtf(fabsf(2.0f * _gravity * _stepHeight));

            btTransform transform = _rigidBody->getWorldTransform();
            transform.setOrigin(transform.getOrigin() + (dt * minStepUpSpeed) * _stepUpVelocity);
            _rigidBody->setWorldTransform(transform);

            // make sure the upward velocity is large enough to clear the very top of the step
            const btScalar MAGIC_STEP_OVERSHOOT_SPEED_COEFFICIENT = 0.5f;
            minStepUpSpeed = MAGIC_STEP_OVERSHOOT_SPEED_COEFFICIENT * sqrtf(fabsf(2.0f * _gravity * _minStepHeight));
            btScalar vDotUp = velocity.dot(_currentUp);
            if (vDotUp < minStepUpSpeed) {
                velocity += (minStepUpSpeed - vDotUp) * _stepUpVelocity;
            }
        }
        _rigidBody->setLinearVelocity(velocity + _parentVelocity);
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
                    _rigidBody->setGravity(_gravity * _currentUp);
                }
            }
        }
        _state = desiredState;
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
        const btScalar DEFAULT_MIN_STEP_HEIGHT = 0.041f; // HACK: hardcoded now but should just larger than shape margin
        const btScalar MAX_STEP_FRACTION_OF_HALF_HEIGHT = 0.56f;
        _minStepHeight = DEFAULT_MIN_STEP_HEIGHT;
        _maxStepHeight = MAX_STEP_FRACTION_OF_HALF_HEIGHT * (_halfHeight + _radius);

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
            _gravity = _collisionGroup == BULLET_COLLISION_GROUP_COLLISIONLESS ? 0.0f : DEFAULT_CHARACTER_GRAVITY;
            _rigidBody->setGravity(_gravity * _currentUp);
        }
    }
}

void CharacterController::updateUpAxis(const glm::quat& rotation) {
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    _ghost.setUpDirection(_currentUp);
    if (_state != State::Hover && _rigidBody) {
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

void CharacterController::setFollowParameters(const glm::mat4& desiredWorldBodyMatrix) {
    _followDesiredBodyTransform = glmToBullet(desiredWorldBodyMatrix) * btTransform(btQuaternion::getIdentity(), glmToBullet(_shapeLocalOffset));
    if (!_following) {
        _following = true;
        _followVelocity = btVector3(0.0f, 0.0f, 0.0f);
    } else if (_followTimeAccumulator > 0.0f) {
        btVector3 newFollowVelocity = (_followDesiredBodyTransform.getOrigin() - _previousFollowPosition) / _followTimeAccumulator;
        const float dontDivideByZero = 0.001f;
        float newSpeed = newFollowVelocity.length() + dontDivideByZero;
        float oldSpeed = _followVelocity.length();

        bool successfulSnap = false;
        btVector3 offset = _followDesiredBodyTransform.getOrigin() - _position;
        const btScalar MAX_FOLLOW_OFFSET = 10.0f * _radius;
        if (offset.length() > MAX_FOLLOW_OFFSET) {
            successfulSnap = queryPenetration(_followDesiredBodyTransform);
            if (successfulSnap) {
                _position = _followDesiredBodyTransform.getOrigin();
            }
        }

        const float VERY_SLOW_HOVER_SPEED = 0.25f;
        const float FAST_CHANGE_SPEED_RATIO = 100.0f;
        if (successfulSnap || (oldSpeed / newSpeed > FAST_CHANGE_SPEED_RATIO && newSpeed < VERY_SLOW_HOVER_SPEED)) {
            // character is snapping to avatar position or avatar is stopping quickly
            // HACK: slam _followVelocity and _rigidBody velocity immediately
            _followVelocity = newFollowVelocity;
            newFollowVelocity.setY(0.0f);
            _rigidBody->setLinearVelocity(newFollowVelocity);
        } else {
            // use simple blending to filter noise from the velocity measurement
            const float blend = 0.2f;
            _followVelocity = (1.0f - blend) * _followVelocity + blend * newFollowVelocity;
        }
    }
    _previousFollowPosition = _followDesiredBodyTransform.getOrigin();
    _followTimeAccumulator = 0.0f;
}

glm::vec3 CharacterController::getLinearVelocity() const {
    glm::vec3 velocity(0.0f);
    if (_rigidBody) {
        velocity = bulletToGLM(_rigidBody->getLinearVelocity());
    }
    return velocity;
}

glm::vec3 CharacterController::getPreActionLinearVelocity() const {
    return _preActionVelocity;
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

        if (_stepHeight > _minStepHeight) {
            // there is a step --> compute velocity direction to go over step
            btVector3 motorVelocityWF = motorVelocity.rotate(axis, angle);
            if (motorVelocityWF.dot(_stepNormal) < 0.0f) {
                // the motor pushes against step
                motorVelocityWF = _stepNormal.cross(_stepPoint.cross(motorVelocityWF));
                btScalar doubleCrossLength2 = motorVelocityWF.length2();
                if (doubleCrossLength2 > FLT_EPSILON) {
                    // scale the motor in the correct direction and rotate back to motor-frame
                    motorVelocityWF *= (motorVelocity.length() / sqrtf(doubleCrossLength2));
                    _stepUpVelocity += motorVelocityWF.rotate(axis, -angle);
                }
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
    _stepUpVelocity = btVector3(0.0f, 0.0f, 0.0f);
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
    const btScalar FLY_TO_GROUND_THRESHOLD = 0.1f * _radius;
    const btScalar GROUND_TO_FLY_THRESHOLD = 0.8f * _radius + _halfHeight;
    const quint64 TAKE_OFF_TO_IN_AIR_PERIOD = 250 * MSECS_PER_SECOND;
    const btScalar MIN_HOVER_HEIGHT = 2.5f;
    const quint64 JUMP_TO_HOVER_PERIOD = 1100 * MSECS_PER_SECOND;

    // scan for distant floor
    // rayStart is at center of bottom sphere
    btVector3 rayStart = _position;

    // rayEnd is straight down MAX_FALL_HEIGHT
    btScalar rayLength = _radius + MAX_FALL_HEIGHT;
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
            btVector3 actualHorizVelocity = velocity - velocity.dot(_currentUp) * _currentUp;
            const btScalar MAX_WALKING_SPEED = 2.5f;
            bool flyingFast = _state == State::Hover && actualHorizVelocity.length() > (MAX_WALKING_SPEED * 0.75f);

            if ((_floorDistance < MIN_HOVER_HEIGHT) && !jumpButtonHeld && !flyingFast) {
                SET_STATE(State::InAir, "near ground");
            } else if (((_floorDistance < FLY_TO_GROUND_THRESHOLD) || _hasSupport) && !flyingFast) {
                SET_STATE(State::Ground, "touching ground");
            }
            break;
        }
        if (_moveKinematically && _ghost.isHovering()) {
            SET_STATE(State::Hover, "kinematic motion"); // HACK
        }
    } else {
        // OUTOFBODY_HACK -- in collisionless state switch only between Ground and Hover states
        if (rayHasHit) {
            SET_STATE(State::Ground, "collisionless above ground");
        } else {
            SET_STATE(State::Hover, "collisionless in air");
        }
    }
}

void CharacterController::preSimulation() {
    if (_dynamicsWorld) {
        // slam body transform and remember velocity
        _rigidBody->setWorldTransform(btTransform(btTransform(_rotation, _position)));
        _preSimulationVelocity = _rigidBody->getLinearVelocity();

        updateState();
    }

    _previousFlags = _pendingFlags;
    _pendingFlags &= ~PENDING_FLAG_JUMP;
}

void CharacterController::postSimulation() {
    _velocityChange = _rigidBody->getLinearVelocity() - _preSimulationVelocity;
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

float CharacterController::measureMaxHipsOffsetRadius(const glm::vec3& currentHipsOffset, float maxSweepDistance) {
    btVector3 hipsOffset = glmToBullet(currentHipsOffset); // rig-frame
    btScalar hipsOffsetLength = hipsOffset.length();
    if (hipsOffsetLength > FLT_EPSILON) {
        const btTransform& transform = _rigidBody->getWorldTransform();

        // rotate into world-frame
        btTransform rotation = transform;
        rotation.setOrigin(btVector3(0.0f, 0.0f, 0.0f));
        btVector3 startPos = transform.getOrigin() - rotation * glmToBullet(_shapeLocalOffset);
        btTransform startTransform = transform;
        startTransform.setOrigin(startPos);
        btVector3 endPos = startPos + rotation * ((maxSweepDistance / hipsOffsetLength) * hipsOffset);

        // sweep test a sphere
        btSphereShape sphere(_radius);
        CharacterSweepResult result(&_ghost);
        btTransform endTransform = startTransform;
        endTransform.setOrigin(endPos);
        _ghost.sweepTest(&sphere, startTransform, endTransform, result);

        // measure sweep success
        if (result.hasHit()) {
            maxSweepDistance *= result.m_closestHitFraction;
        }
    }
    return maxSweepDistance;
}

void CharacterController::setMoveKinematically(bool kinematic) {
    if (kinematic != _moveKinematically) {
        _moveKinematically = kinematic;
        _pendingFlags |= PENDING_FLAG_UPDATE_SHAPE;
        _ghost.setMotorOnly(!_moveKinematically);
    }
}

bool CharacterController::queryPenetration(const btTransform& transform) {
    btVector3 minBox;
    btVector3 maxBox;
    _ghost.setWorldTransform(transform);
    _ghost.measurePenetration(minBox, maxBox);
    btVector3 penetration = minBox;
    penetration.setMax(maxBox.absolute());
    const btScalar MIN_PENETRATION_SQUARED = 0.0016f; // 0.04^2
    return penetration.length2() < MIN_PENETRATION_SQUARED;
}
