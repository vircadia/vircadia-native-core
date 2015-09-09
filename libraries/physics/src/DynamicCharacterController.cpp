#include <BulletCollision/CollisionShapes/btMultiSphereShape.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <LinearMath/btDefaultMotionState.h>

#include <PhysicsCollisionGroups.h>

#include "BulletUtil.h"
#include "DynamicCharacterController.h"
#include "PhysicsLogging.h"

const btVector3 LOCAL_UP_AXIS(0.0f, 1.0f, 0.0f);
const float DEFAULT_GRAVITY = -5.0f;
const float JUMP_SPEED = 3.5f;

const float MAX_FALL_HEIGHT = 20.0f;

const uint32_t PENDING_FLAG_ADD_TO_SIMULATION = 1U << 0;
const uint32_t PENDING_FLAG_REMOVE_FROM_SIMULATION = 1U << 1;
const uint32_t PENDING_FLAG_UPDATE_SHAPE = 1U << 2;
const uint32_t PENDING_FLAG_JUMP = 1U << 3;

// TODO: improve walking up steps
// TODO: make avatars able to walk up and down steps/slopes
// TODO: make avatars stand on steep slope
// TODO: make avatars not snag on low ceilings

// helper class for simple ray-traces from character
class ClosestNotMe : public btCollisionWorld::ClosestRayResultCallback {
public:
    ClosestNotMe(btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
        _me = me;
    }
    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) {
        if (rayResult.m_collisionObject == _me) {
            return 1.0f;
        }
        return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace
    );
}
protected:
    btRigidBody* _me;
};

DynamicCharacterController::DynamicCharacterController(AvatarData* avatarData) {
    _halfHeight = 1.0f;
    _shape = nullptr;
    _rigidBody = nullptr;

    assert(avatarData);
    _avatarData = avatarData;

    _enabled = false;

    _floorDistance = MAX_FALL_HEIGHT;

    _walkVelocity.setValue(0.0f,0.0f,0.0f);
    _jumpSpeed = JUMP_SPEED;
    _isOnGround = false;
    _isJumping = false;
    _isFalling = false;
    _isHovering = true;
    _isPushingUp = false;
    _jumpToHoverStart = 0;

    _pendingFlags = PENDING_FLAG_UPDATE_SHAPE;
    updateShapeIfNecessary();
}

DynamicCharacterController::~DynamicCharacterController() {
}

// virtual 
void DynamicCharacterController::setWalkDirection(const btVector3& walkDirection) {
    // do nothing -- walkVelocity is upated in preSimulation()
    //_walkVelocity = walkDirection;
}

void DynamicCharacterController::preStep(btCollisionWorld* collisionWorld) {
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

void DynamicCharacterController::playerStep(btCollisionWorld* dynaWorld, btScalar dt) {
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
}

void DynamicCharacterController::jump() {
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

bool DynamicCharacterController::onGround() const {
    const btScalar FLOOR_PROXIMITY_THRESHOLD = 0.3f * _radius;
    return _floorDistance < FLOOR_PROXIMITY_THRESHOLD;
}

void DynamicCharacterController::setHovering(bool hover) {
    if (hover != _isHovering) {
        _isHovering = hover;
        _isJumping = false;

        if (_rigidBody) {
            if (hover) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(DEFAULT_GRAVITY * _currentUp);
            }
        }
    }
}

void DynamicCharacterController::setLocalBoundingBox(const glm::vec3& corner, const glm::vec3& scale) {
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

bool DynamicCharacterController::needsRemoval() const {
    return (bool)(_pendingFlags & PENDING_FLAG_REMOVE_FROM_SIMULATION);
}

bool DynamicCharacterController::needsAddition() const {
    return (bool)(_pendingFlags & PENDING_FLAG_ADD_TO_SIMULATION);
}

void DynamicCharacterController::setEnabled(bool enabled) {
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

void DynamicCharacterController::setDynamicsWorld(btDynamicsWorld* world) {
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
            _pendingFlags &= ~ PENDING_FLAG_JUMP;
            _dynamicsWorld->addRigidBody(_rigidBody, COLLISION_GROUP_MY_AVATAR, COLLISION_MASK_MY_AVATAR);
            _dynamicsWorld->addAction(this);
            //reset(_dynamicsWorld);
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
        _pendingFlags &= ~ PENDING_FLAG_REMOVE_FROM_SIMULATION;
    }
}

void DynamicCharacterController::updateShapeIfNecessary() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_SHAPE) {
        // make sure there is NO pending removal from simulation at this point
        // (don't want to delete _rigidBody out from under the simulation)
        assert(!(_pendingFlags & PENDING_FLAG_REMOVE_FROM_SIMULATION));
        _pendingFlags &= ~ PENDING_FLAG_UPDATE_SHAPE;
        // delete shape and RigidBody
        delete _rigidBody;
        _rigidBody = nullptr;
        delete _shape;
        _shape = nullptr;

        // compute new dimensions from avatar's bounding box
        float x = _boxScale.x;
        float z = _boxScale.z;
        _radius = 0.5f * sqrtf(0.5f * (x * x + z * z));
        _halfHeight = 0.5f * _boxScale.y - _radius;
        float MIN_HALF_HEIGHT = 0.1f;
        if (_halfHeight < MIN_HALF_HEIGHT) {
            _halfHeight = MIN_HALF_HEIGHT;
        }
        // NOTE: _shapeLocalOffset is already computed

        if (_radius > 0.0f) {
            // create new shape
            _shape = new btCapsuleShape(_radius, 2.0f * _halfHeight);

            // HACK: use some simple mass property defaults for now
            float mass = 100.0f;
            btVector3 inertia(30.0f, 8.0f, 30.0f);

            // create new body
            _rigidBody = new btRigidBody(mass, nullptr, _shape, inertia);
            _rigidBody->setSleepingThresholds(0.0f, 0.0f);
            _rigidBody->setAngularFactor(0.0f);
            _rigidBody->setWorldTransform(btTransform(glmToBullet(_avatarData->getOrientation()),
                                                      glmToBullet(_avatarData->getPosition())));
            if (_isHovering) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(DEFAULT_GRAVITY * _currentUp);
            }
            //_rigidBody->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
        } else {
            // TODO: handle this failure case
        }
    }
}

void DynamicCharacterController::updateUpAxis(const glm::quat& rotation) {
    btVector3 oldUp = _currentUp;
    _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
    if (!_isHovering) {
        const btScalar MIN_UP_ERROR = 0.01f;
        if (oldUp.distance(_currentUp) > MIN_UP_ERROR) {
            _rigidBody->setGravity(DEFAULT_GRAVITY * _currentUp);
        }
    }
}

void DynamicCharacterController::preSimulation(btScalar timeStep) {
    if (_enabled && _dynamicsWorld) {
        glm::quat rotation = _avatarData->getOrientation();

        // TODO: update gravity if up has changed
        updateUpAxis(rotation);

        glm::vec3 position = _avatarData->getPosition() + rotation * _shapeLocalOffset;
        _rigidBody->setWorldTransform(btTransform(glmToBullet(rotation), glmToBullet(position)));

        // the rotation is dictated by AvatarData
        btTransform xform = _rigidBody->getWorldTransform();
        xform.setRotation(glmToBullet(rotation));
        _rigidBody->setWorldTransform(xform);

        // scan for distant floor
        // rayStart is at center of bottom sphere
        btVector3 rayStart = xform.getOrigin() - _halfHeight * _currentUp;
    
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

        _walkVelocity = glmToBullet(_avatarData->getTargetVelocity());

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
}

void DynamicCharacterController::postSimulation() {
    if (_enabled && _rigidBody) {
        const btTransform& avatarTransform = _rigidBody->getWorldTransform();
        glm::quat rotation = bulletToGLM(avatarTransform.getRotation());
        glm::vec3 position = bulletToGLM(avatarTransform.getOrigin());

        _avatarData->nextAttitude(position - rotation * _shapeLocalOffset, rotation);
        _avatarData->setVelocity(bulletToGLM(_rigidBody->getLinearVelocity()));
    }
}
