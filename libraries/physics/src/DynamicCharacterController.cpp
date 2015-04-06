#include <BulletCollision/CollisionShapes/btMultiSphereShape.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <LinearMath/btDefaultMotionState.h>

#include "BulletUtil.h"
#include "DynamicCharacterController.h"

const btVector3 LOCAL_UP_AXIS(0.0f, 1.0f, 0.0f);
const float DEFAULT_GRAVITY = 5.0f;
const float TERMINAL_VELOCITY = 55.0f;
const float JUMP_SPEED = 3.5f;

const uint32_t PENDING_FLAG_ADD_TO_SIMULATION = 1U << 0;
const uint32_t PENDING_FLAG_REMOVE_FROM_SIMULATION = 1U << 1;
const uint32_t PENDING_FLAG_UPDATE_SHAPE = 1U << 2;
const uint32_t PENDING_FLAG_JUMP = 1U << 3;

DynamicCharacterController::DynamicCharacterController(AvatarData* avatarData) {
    _rayLambda[0] = 1.0f;
    _rayLambda[1] = 1.0f;
    _halfHeight = 1.0f;
    //_turnVelocity = 1.0f; // radians/sec
    _shape = NULL;
    _rigidBody = NULL;

    assert(avatarData);
    _avatarData = avatarData;

    _enabled = false;

    _walkVelocity.setValue(0.0f,0.0f,0.0f);
    //_verticalVelocity = 0.0f;
    _jumpSpeed = JUMP_SPEED;
    _isOnGround = false;
    _isJumping = false;
    _isHovering = true;

    _pendingFlags = PENDING_FLAG_UPDATE_SHAPE;
    updateShapeIfNecessary();
}

DynamicCharacterController::~DynamicCharacterController() {
}

// virtual 
void DynamicCharacterController::setWalkDirection(const btVector3& walkDirection) {
    _walkVelocity = walkDirection;
}

/*
void DynamicCharacterController::setup(btScalar height, btScalar width, btScalar stepHeight) {
    btVector3 spherePositions[2];
    btScalar sphereRadii[2];
    
    sphereRadii[0] = width;
    sphereRadii[1] = width;
    spherePositions[0] = btVector3 (0.0f, (height/btScalar(2.0f) - width), 0.0f);
    spherePositions[1] = btVector3 (0.0f, (-height/btScalar(2.0f) + width), 0.0f);

    _halfHeight = height/btScalar(2.0f);

    _shape = new btMultiSphereShape(&spherePositions[0], &sphereRadii[0], 2);

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(0.0f, 2.0f, 0.0f));
    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo cInfo(1.0f, myMotionState, _shape);
    _rigidBody = new btRigidBody(cInfo);
    // kinematic vs. static doesn't work
    //_rigidBody->setCollisionFlags( _rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    _rigidBody->setSleepingThresholds(0.0f, 0.0f);
    _rigidBody->setAngularFactor(0.0f);
}

void DynamicCharacterController::destroy() {
    if (_shape) {
        delete _shape;
    }

    if (_rigidBody) {
        delete _rigidBody;
        _rigidBody = NULL;
    }
}
*/

btCollisionObject* DynamicCharacterController::getCollisionObject() {
    return _rigidBody;
}

void DynamicCharacterController::preStep(btCollisionWorld* collisionWorld) {
    const btTransform& xform = _rigidBody->getCenterOfMassTransform();

    btVector3 down = -xform.getBasis()[1];
    btVector3 forward = xform.getBasis()[2];
    down.normalize();
    forward.normalize();

    _raySource[0] = xform.getOrigin();
    _raySource[1] = xform.getOrigin();

    _rayTarget[0] = _raySource[0] + down * _halfHeight * btScalar(1.1f);
    _rayTarget[1] = _raySource[1] + forward * _halfHeight * btScalar(1.1f);

    class ClosestNotMe : public btCollisionWorld::ClosestRayResultCallback {
    public:
        ClosestNotMe(btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
            _me = me;
        }

        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) {
            if (rayResult.m_collisionObject == _me)
                return 1.0f;

            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace
        );
    }
    protected:
        btRigidBody* _me;
    };

    ClosestNotMe rayCallback(_rigidBody);

    int i = 0;
    for (i = 0; i < 2; i++) {
        rayCallback.m_closestHitFraction = 1.0f;
        collisionWorld->rayTest(_raySource[i], _rayTarget[i], rayCallback);
        if (rayCallback.hasHit()) {
            _rayLambda[i] = rayCallback.m_closestHitFraction;
        } else {
            _rayLambda[i] = 1.0f;
        }
    }
}

void DynamicCharacterController::playerStep(btCollisionWorld* dynaWorld,btScalar dt) {
    /* Handle turning 
    const btTransform& xform = _rigidBody->getCenterOfMassTransform();
    if (left)
        _turnAngle -= dt * _turnVelocity;
    if (right)
        _turnAngle += dt * _turnVelocity;

    xform.setRotation(btQuaternion(btVector3(0.0, 1.0, 0.0), _turnAngle));
    */

    btVector3 currentVelocity = _rigidBody->getLinearVelocity();
    btScalar currentSpeed = currentVelocity.length();

    btVector3 desiredVelocity = _walkVelocity;
    btScalar desiredSpeed = desiredVelocity.length();
    const btScalar MIN_SPEED = 0.001f;
    if (desiredSpeed < MIN_SPEED) {
        if (currentSpeed < MIN_SPEED) {
            _rigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
        } else {
            const btScalar BRAKING_TIMESCALE = 0.2f;
            btScalar tau = dt / BRAKING_TIMESCALE;
            _rigidBody->setLinearVelocity((1.0f - tau) * currentVelocity);
        }
    } else {
        const btScalar WALKING_TIMESCALE = 0.5f;
        btScalar tau = dt / WALKING_TIMESCALE;
        if (onGround()) {
            // subtract vertical component
            desiredVelocity = desiredVelocity - desiredVelocity.dot(_currentUp) * _currentUp;
        }
        _rigidBody->setLinearVelocity(currentVelocity - tau * (currentVelocity - desiredVelocity));
    }

    /*
    _rigidBody->getMotionState()->setWorldTransform(xform);
    _rigidBody->setCenterOfMassTransform(xform);
    */
}

bool DynamicCharacterController::canJump() const {
    return false; // temporarily disabled
    //return onGround();
}

void DynamicCharacterController::jump() {
    /*
    if (!canJump()) {
        return;
    }

    btTransform xform = _rigidBody->getCenterOfMassTransform();
    btVector3 up = xform.getBasis()[1];
    up.normalize();
    btScalar magnitude = (btScalar(1.0)/_rigidBody->getInvMass()) * btScalar(8.0);
    _rigidBody->applyCentralImpulse(up * magnitude);
    */
}

bool DynamicCharacterController::onGround() const {
    return _rayLambda[0] < btScalar(1.0);
}

void DynamicCharacterController::debugDraw(btIDebugDraw* debugDrawer) {
}

void DynamicCharacterController::setUpInterpolate(bool value) {
    // This method is required by btCharacterControllerInterface, but it does nothing.
    // What it used to do was determine whether stepUp() would: stop where it hit the ceiling
    // (interpolate = true, and now default behavior) or happily penetrate objects above the avatar.
}

void DynamicCharacterController::warp(const btVector3& origin) {
}

void DynamicCharacterController::reset(btCollisionWorld* foo) {
}

void DynamicCharacterController::registerPairCacheAndDispatcher(btOverlappingPairCache* pairCache, btCollisionDispatcher* dispatcher) {
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
            _isHovering = true;
        } else {
            if (_dynamicsWorld) {
                _pendingFlags |= PENDING_FLAG_REMOVE_FROM_SIMULATION;
            }
            _pendingFlags &= ~ PENDING_FLAG_ADD_TO_SIMULATION;
            _isOnGround = false;
        }
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
            _dynamicsWorld = NULL;
        }
        if (world && _rigidBody) {
            _dynamicsWorld = world;
            _pendingFlags &= ~ PENDING_FLAG_JUMP;
            _dynamicsWorld->addRigidBody(_rigidBody);
//            _dynamicsWorld->addCollisionObject(_rigidBody,
//                    btBroadphaseProxy::CharacterFilter,
//                    btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
            _dynamicsWorld->addAction(this);
//            reset(_dynamicsWorld);
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
        // delete shape and GhostObject
        delete _rigidBody;
        _rigidBody = NULL;
        delete _shape;
        _shape = NULL;

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

            // create new body
            float mass = 1.0f;
            btVector3 inertia(1.0f, 1.0f, 1.0f);
            _rigidBody = new btRigidBody(mass, NULL, _shape, inertia);
            _rigidBody->setSleepingThresholds (0.0f, 0.0f);
            _rigidBody->setAngularFactor (0.0f);
            _rigidBody->setWorldTransform(btTransform(glmToBullet(_avatarData->getOrientation()),
                                                            glmToBullet(_avatarData->getPosition())));
            // stepHeight affects the heights of ledges that the character can ascend
            //_stepUpHeight = _radius + 0.25f * _halfHeight + 0.1f;
            //_stepDownHeight = _radius;

            //_rigidBody->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
        } else {
            // TODO: handle this failure case
        }
    }
}

void DynamicCharacterController::preSimulation(btScalar timeStep) {
    if (_enabled && _dynamicsWorld) {
        glm::quat rotation = _avatarData->getOrientation();
        _currentUp = quatRotate(glmToBullet(rotation), LOCAL_UP_AXIS);
        glm::vec3 position = _avatarData->getPosition() + rotation * _shapeLocalOffset;

        // TODO: get intended WALK velocity from _avatarData, not its actual velocity
        btVector3 walkVelocity = glmToBullet(_avatarData->getVelocity());

        _rigidBody->setWorldTransform(btTransform(glmToBullet(rotation), glmToBullet(position)));
        _rigidBody->setLinearVelocity(walkVelocity);
        //setVelocityForTimeInterval(walkVelocity, timeStep);
        if (_pendingFlags & PENDING_FLAG_JUMP) {
            _pendingFlags &= ~ PENDING_FLAG_JUMP;
            if (canJump()) {
                //_verticalVelocity = _jumpSpeed;
                _isJumping = true;
            }
        }
        // remember last position so we can throttle the total motion from the next step
//        _lastPosition = position;
//        _stepDt = 0.0f;

        // the rotation is determined by AvatarData
        btTransform xform = _rigidBody->getCenterOfMassTransform();
        xform.setRotation(glmToBullet(rotation));
        _rigidBody->setCenterOfMassTransform(xform);
    }
}

void DynamicCharacterController::postSimulation() {
    if (_enabled && _rigidBody) {
        const btTransform& avatarTransform = _rigidBody->getCenterOfMassTransform();
        glm::quat rotation = bulletToGLM(avatarTransform.getRotation());
        glm::vec3 position = bulletToGLM(avatarTransform.getOrigin());

        /*
        // cap the velocity of the step so that the character doesn't POP! so hard on steps
        glm::vec3 finalStep = position - _lastPosition;
        btVector3 finalVelocity = _walkVelocity + _verticalVelocity * _currentUp;;
        const btScalar MAX_RESOLUTION_SPEED = 5.0f; // m/sec
        btScalar maxStepLength = glm::max(MAX_RESOLUTION_SPEED, 2.0f * finalVelocity.length()) * _stepDt;
        btScalar stepLength = glm::length(finalStep);
        if (stepLength > maxStepLength) {
            position = _lastPosition + (maxStepLength / stepLength) * finalStep;
            // NOTE: we don't need to move ghostObject to throttled position unless 
            // we want to support do async ray-traces/collision-queries against character
        }
        */

        _avatarData->setOrientation(rotation);
        _avatarData->setPosition(position - rotation * _shapeLocalOffset);
    }
}

