//
//  MyCharacterController.h
//  interface/src/avatar
//
//  Created by AndrewMeadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MyCharacterController.h"

#include <BulletUtil.h>
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"

#include "MyAvatar.h"
#include "DetailedMotionState.h"

// TODO: make avatars stand on steep slope
// TODO: make avatars not snag on low ceilings


void MyCharacterController::RayShotgunResult::reset() {
    hitFraction = 1.0f;
    walkable = true;
}

MyCharacterController::MyCharacterController(std::shared_ptr<MyAvatar> avatar,
                                             const FollowTimePerType& followTimeRemainingPerType) :
    CharacterController(followTimeRemainingPerType) {

    assert(avatar);
    _avatar = avatar;
    updateShapeIfNecessary();
}

MyCharacterController::~MyCharacterController() {
}

void MyCharacterController::addToWorld() {
    CharacterController::addToWorld();
    if (_rigidBody) {
        initRayShotgun(_physicsEngine->getDynamicsWorld());
    }
}

void MyCharacterController::updateShapeIfNecessary() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_SHAPE) {
        _pendingFlags &= ~PENDING_FLAG_UPDATE_SHAPE;
        if (_radius > 0.0f) {
            // create RigidBody if it doesn't exist
            if (!_rigidBody) {
                btCollisionShape* shape = computeShape();
                btScalar mass = 1.0f;
                btVector3 inertia(1.0f, 1.0f, 1.0f);
                _rigidBody = new btRigidBody(mass, nullptr, shape, inertia);
            } else {
                btCollisionShape* shape = _rigidBody->getCollisionShape();
                if (shape) {
                    delete shape;
                }
                shape = computeShape();
                _rigidBody->setCollisionShape(shape);
            }
            updateMassProperties();

            _rigidBody->setSleepingThresholds(0.0f, 0.0f);
            _rigidBody->setAngularFactor(0.0f);
            _rigidBody->setWorldTransform(btTransform(glmToBullet(_avatar->getWorldOrientation()),
                                                      glmToBullet(_avatar->getWorldPosition())));
            _rigidBody->setDamping(0.0f, 0.0f);
            if (_state == State::Hover) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(_gravity * _currentUp);
            }
            _rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() &
                    ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT));
        } else {
            // TODO: handle this failure case
        }
    }
}

bool MyCharacterController::testRayShotgun(const glm::vec3& position, const glm::vec3& step, RayShotgunResult& result) {
    btVector3 rayDirection = glmToBullet(step);
    btScalar stepLength = rayDirection.length();
    if (stepLength < FLT_EPSILON) {
        return false;
    }
    rayDirection /= stepLength;

    // get _ghost ready for ray traces
    btTransform transform = _rigidBody->getWorldTransform();
    btVector3 newPosition = glmToBullet(position);
    transform.setOrigin(newPosition);
    _ghost.setWorldTransform(transform);
    btMatrix3x3 rotation = transform.getBasis();
    _ghost.refreshOverlappingPairCache();

    CharacterRayResult rayResult(&_ghost);
    CharacterRayResult closestRayResult(&_ghost);
    btVector3 rayStart;
    btVector3 rayEnd;

    // compute rotation that will orient local ray start points to face step direction
    btVector3 forward = rotation * btVector3(0.0f, 0.0f, -1.0f);
    btVector3 adjustedDirection = rayDirection - rayDirection.dot(_currentUp) * _currentUp;
    btVector3 axis = forward.cross(adjustedDirection);
    btScalar lengthAxis = axis.length();
    if (lengthAxis > FLT_EPSILON) {
        // we're walking sideways
        btScalar cosAngle = lengthAxis / adjustedDirection.length();
        if (cosAngle < 1.0f) {
            btScalar angle = acosf(cosAngle);
            if (rayDirection.dot(forward) < 0.0f) {
                angle = PI - angle;
            }
            axis /= lengthAxis;
            rotation = btMatrix3x3(btQuaternion(axis, angle)) * rotation;
        }
    } else if (rayDirection.dot(forward) < 0.0f) {
        // we're walking backwards
        rotation = btMatrix3x3(btQuaternion(_currentUp, PI)) * rotation;
    }

    // scan the top
    // NOTE: if we scan an extra distance forward we can detect flat surfaces that are too steep to walk on.
    // The approximate extra distance can be derived with trigonometry.
    //
    //   minimumForward = [ (maxStepHeight + radius / cosTheta - radius) * (cosTheta / sinTheta) - radius ]
    //
    // where: theta = max angle between floor normal and vertical
    //
    // if stepLength is not long enough we can add the difference.
    //
    btScalar cosTheta = _minFloorNormalDotUp;
    btScalar sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const btScalar MIN_FORWARD_SLOP = 0.12f; // HACK: not sure why this is necessary to detect steepest walkable slope
    btScalar forwardSlop = (_maxStepHeight + _radius / cosTheta - _radius) * (cosTheta / sinTheta) - (_radius + stepLength) + MIN_FORWARD_SLOP;
    if (forwardSlop < 0.0f) {
        // BIG step, no slop necessary
        forwardSlop = 0.0f;
    }

    const btScalar backSlop = 0.04f;
    for (int32_t i = 0; i < _topPoints.size(); ++i) {
        rayStart = newPosition + rotation * _topPoints[i] - backSlop * rayDirection;
        rayEnd = rayStart + (backSlop + stepLength + forwardSlop) * rayDirection;
        if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
            if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                closestRayResult = rayResult;
            }
            if (result.walkable) {
                if (rayResult.m_hitNormalWorld.dot(_currentUp) < _minFloorNormalDotUp) {
                    result.walkable = false;
                    // the top scan wasn't walkable so don't bother scanning the bottom
                    // remove both forwardSlop and backSlop
                    result.hitFraction = glm::min(1.0f, (closestRayResult.m_closestHitFraction * (backSlop + stepLength + forwardSlop) - backSlop) / stepLength);
                    return result.hitFraction < 1.0f;
                }
            }
        }
    }
    if (_state == State::Hover) {
        // scan the bottom just like the top
        for (int32_t i = 0; i < _bottomPoints.size(); ++i) {
            rayStart = newPosition + rotation * _bottomPoints[i] - backSlop * rayDirection;
            rayEnd = rayStart + (backSlop + stepLength + forwardSlop) * rayDirection;
            if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
                if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                    closestRayResult = rayResult;
                }
                if (result.walkable) {
                    if (rayResult.m_hitNormalWorld.dot(_currentUp) < _minFloorNormalDotUp) {
                        result.walkable = false;
                        // the bottom scan wasn't walkable
                        // remove both forwardSlop and backSlop
                        result.hitFraction = glm::min(1.0f, (closestRayResult.m_closestHitFraction * (backSlop + stepLength + forwardSlop) - backSlop) / stepLength);
                        return result.hitFraction < 1.0f;
                    }
                }
            }
        }
    } else {
        // scan the bottom looking for nearest step point
        // remove forwardSlop
        result.hitFraction = (closestRayResult.m_closestHitFraction * (backSlop + stepLength + forwardSlop)) / (backSlop + stepLength);

        for (int32_t i = 0; i < _bottomPoints.size(); ++i) {
            rayStart = newPosition + rotation * _bottomPoints[i] - backSlop * rayDirection;
            rayEnd = rayStart + (backSlop + stepLength) * rayDirection;
            if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
                if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                    closestRayResult = rayResult;
                }
            }
        }
        // remove backSlop
        // NOTE: backSlop removal can produce a NEGATIVE hitFraction!
        // which means the shape is actually in interpenetration
        result.hitFraction = ((closestRayResult.m_closestHitFraction * (backSlop + stepLength)) - backSlop) / stepLength;
    }
    return result.hitFraction < 1.0f;
}

int32_t MyCharacterController::computeCollisionMask() const {
    int32_t collisionMask = BULLET_COLLISION_MASK_MY_AVATAR;
    if (_collisionless && _collisionlessAllowed) {
        collisionMask = BULLET_COLLISION_MASK_COLLISIONLESS;
    } else if (!_collideWithOtherAvatars) {
        collisionMask &= ~BULLET_COLLISION_GROUP_OTHER_AVATAR;
    }
    return collisionMask;
}

void MyCharacterController::handleChangedCollisionMask() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_COLLISION_MASK) {
        // ATM the easiest way to update collision groups/masks is to remove/re-add the RigidBody
        // but we don't do it here.  Instead we set some flags to remind us to do it later.
        _pendingFlags |= (PENDING_FLAG_REMOVE_FROM_SIMULATION | PENDING_FLAG_ADD_TO_SIMULATION);
        _pendingFlags &= ~PENDING_FLAG_UPDATE_COLLISION_MASK;
        updateCurrentGravity();
    }
}

bool MyCharacterController::needsSafeLandingSupport() const {
    return _isStuck && _numStuckSubsteps >= NUM_SUBSTEPS_FOR_SAFE_LANDING_RETRY;
}

btConvexHullShape* MyCharacterController::computeShape() const {
    // HACK: the avatar collides using convex hull with a collision margin equal to
    // the old capsule radius.  Two points define a capsule and additional points are
    // spread out at chest level to produce a slight taper toward the feet.  This
    // makes the avatar more likely to collide with vertical walls at a higher point
    // and thus less likely to produce a single-point collision manifold below the
    // _maxStepHeight when walking into against vertical surfaces --> fixes a bug
    // where the "walk up steps" feature would allow the avatar to walk up vertical
    // walls.
    const int32_t NUM_POINTS = 6;
    btVector3 points[NUM_POINTS];
    btVector3 xAxis = btVector3(1.0f, 0.0f, 0.0f);
    btVector3 yAxis = btVector3(0.0f, 1.0f, 0.0f);
    btVector3 zAxis = btVector3(0.0f, 0.0f, 1.0f);
    points[0] = _halfHeight * yAxis;
    points[1] = -_halfHeight * yAxis;
    points[2] = (0.75f * _halfHeight) * yAxis - (0.1f * _radius) * zAxis;
    points[3] = (0.75f * _halfHeight) * yAxis + (0.1f * _radius) * zAxis;
    points[4] = (0.75f * _halfHeight) * yAxis - (0.1f * _radius) * xAxis;
    points[5] = (0.75f * _halfHeight) * yAxis + (0.1f * _radius) * xAxis;
    btConvexHullShape* shape = new btConvexHullShape(reinterpret_cast<btScalar*>(points), NUM_POINTS);
    shape->setMargin(_radius);
    return shape;
}

void MyCharacterController::initRayShotgun(const btCollisionWorld* world) {
    // In order to trace rays out from the avatar's shape surface we need to know where the start points are in
    // the local-frame.  Since the avatar shape is somewhat irregular computing these points by hand is a hassle
    // so instead we ray-trace backwards to the avatar to find them.
    //
    // We trace back a regular grid (see below) of points against the shape and keep any that hit.
    //        ___
    //     + / + \ +
    //      |+   +|
    //     +|  +  | +
    //      |+   +|
    //     +|  +  | +
    //      |+   +|
    //     + \ + /  +
    //        ---
    // The shotgun will send rays out from these same points to see if the avatar's shape can proceed through space.

    // helper class for simple ray-traces against character
    class MeOnlyResultCallback : public btCollisionWorld::ClosestRayResultCallback {
    public:
        MeOnlyResultCallback (btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
            _me = me;
            m_collisionFilterGroup = BULLET_COLLISION_GROUP_DYNAMIC;
            m_collisionFilterMask = BULLET_COLLISION_MASK_DYNAMIC;
        }
        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) override {
            if (rayResult.m_collisionObject != _me) {
                return 1.0f;
            }
            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
        btRigidBody* _me;
    };

    const btScalar fullHalfHeight = _radius + _halfHeight;
    const btScalar divisionLine = -fullHalfHeight + _maxStepHeight; // line between top and bottom
    const btScalar topHeight = fullHalfHeight - divisionLine;
    const btScalar slop = 0.02f;

    const int32_t NUM_ROWS = 5; // must be odd number > 1
    const int32_t NUM_COLUMNS = 5; // must be odd number > 1
    btVector3 reach = (2.0f * _radius) * btVector3(0.0f, 0.0f, 1.0f);

    { // top points
        _topPoints.clear();
        _topPoints.reserve(NUM_ROWS * NUM_COLUMNS);
        btScalar stepY = (topHeight - slop) / (btScalar)(NUM_ROWS - 1);
        btScalar stepX = 2.0f * (_radius - slop) / (btScalar)(NUM_COLUMNS - 1);

        btTransform transform = _rigidBody->getWorldTransform();
        btVector3 position = transform.getOrigin();
        btMatrix3x3 rotation = transform.getBasis();

        for (int32_t i = 0; i < NUM_ROWS; ++i) {
            int32_t maxJ = NUM_COLUMNS;
            btScalar offsetX = -(btScalar)((NUM_COLUMNS - 1) / 2) * stepX;
            if (i % 2 == 1) {
                // odd rows have one less point and start a halfStep closer
                maxJ -= 1;
                offsetX += 0.5f * stepX;
            }
            for (int32_t j = 0; j < maxJ; ++j) {
                btVector3 localRayEnd(offsetX + (btScalar)(j) * stepX, divisionLine + (btScalar)(i) * stepY, 0.0f);
                btVector3 localRayStart = localRayEnd - reach;
                MeOnlyResultCallback result(_rigidBody);
                world->rayTest(position + rotation * localRayStart, position + rotation * localRayEnd, result);
                if (result.m_closestHitFraction < 1.0f) {
                    _topPoints.push_back(localRayStart + result.m_closestHitFraction * reach);
                }
            }
        }
    }

    { // bottom points
        _bottomPoints.clear();
        _bottomPoints.reserve(NUM_ROWS * NUM_COLUMNS);

        btScalar steepestStepHitHeight = (_radius + 0.04f) * (1.0f - DEFAULT_MIN_FLOOR_NORMAL_DOT_UP);
        btScalar stepY = (_maxStepHeight - slop - steepestStepHitHeight) / (btScalar)(NUM_ROWS - 1);
        btScalar stepX = 2.0f * (_radius - slop) / (btScalar)(NUM_COLUMNS - 1);

        btTransform transform = _rigidBody->getWorldTransform();
        btVector3 position = transform.getOrigin();
        btMatrix3x3 rotation = transform.getBasis();

        for (int32_t i = 0; i < NUM_ROWS; ++i) {
            int32_t maxJ = NUM_COLUMNS;
            btScalar offsetX = -(btScalar)((NUM_COLUMNS - 1) / 2) * stepX;
            if (i % 2 == 1) {
                // odd rows have one less point and start a halfStep closer
                maxJ -= 1;
                offsetX += 0.5f * stepX;
            }
            for (int32_t j = 0; j < maxJ; ++j) {
                btVector3 localRayEnd(offsetX + (btScalar)(j) * stepX, (divisionLine - slop) - (btScalar)(i) * stepY, 0.0f);
                btVector3 localRayStart = localRayEnd - reach;
                MeOnlyResultCallback result(_rigidBody);
                world->rayTest(position + rotation * localRayStart, position + rotation * localRayEnd, result);
                if (result.m_closestHitFraction < 1.0f) {
                    _bottomPoints.push_back(localRayStart + result.m_closestHitFraction * reach);
                }
            }
        }
    }
}

void MyCharacterController::updateMassProperties() {
    assert(_rigidBody);
    // the inertia tensor of a capsule with Y-axis of symmetry, radius R and cylinder height H is:
    // Ix = density * (volumeCylinder * (H^2 / 12 + R^2 / 4) + volumeSphere * (2R^2 / 5 + H^2 / 2 + 3HR / 8))
    // Iy = density * (volumeCylinder * (R^2 / 2) + volumeSphere * (2R^2 / 5)
    btScalar r2 = _radius * _radius;
    btScalar h2 = 4.0f * _halfHeight * _halfHeight;
    btScalar volumeSphere = 4.0f * PI * r2 * _radius / 3.0f;
    btScalar volumeCylinder = TWO_PI * r2 * 2.0f * _halfHeight;
    btScalar cylinderXZ = volumeCylinder * (h2 / 12.0f + r2 / 4.0f);
    btScalar capsXZ = volumeSphere * (2.0f * r2 / 5.0f + h2 / 2.0f + 6.0f * _halfHeight * _radius / 8.0f);
    btScalar inertiaXZ = _density * (cylinderXZ + capsXZ);
    btScalar inertiaY = _density * ((volumeCylinder * r2 / 2.0f) + volumeSphere * (2.0f * r2 / 5.0f));
    btVector3 inertia(inertiaXZ, inertiaY, inertiaXZ);

    btScalar mass = _density * (volumeCylinder + volumeSphere);

    _rigidBody->setMassProps(mass, inertia);
}

const btCollisionShape* MyCharacterController::createDetailedCollisionShapeForJoint(int32_t jointIndex) {
    ShapeInfo shapeInfo;
    _avatar->computeDetailedShapeInfo(shapeInfo, jointIndex);
    if (shapeInfo.getType() != SHAPE_TYPE_NONE) {
        const btCollisionShape* shape = ObjectMotionState::getShapeManager()->getShape(shapeInfo);
        return shape;
    }
    return nullptr;
}

DetailedMotionState* MyCharacterController::createDetailedMotionStateForJoint(int32_t jointIndex) {
    const btCollisionShape* shape = createDetailedCollisionShapeForJoint(jointIndex);
    if (shape) {
        DetailedMotionState* motionState = new DetailedMotionState(_avatar, shape, jointIndex);
        motionState->setMass(_avatar->computeMass());
        return motionState;
    }
    return nullptr;
}

void MyCharacterController::clearDetailedMotionStates() {
    // we don't actually clear the MotionStates here
    // instead we twiddle some flags as a signal of what to do later
    _pendingFlags |= PENDING_FLAG_REMOVE_DETAILED_FROM_SIMULATION; 
    // We make sure we don't add them again
    _pendingFlags &= ~PENDING_FLAG_ADD_DETAILED_TO_SIMULATION;
}

void MyCharacterController::buildPhysicsTransaction(PhysicsEngine::Transaction& transaction) {
    for (auto& detailedMotionState : _detailedMotionStates) {
        detailedMotionState->forceActive();
    }
    if (_pendingFlags & PENDING_FLAG_REMOVE_DETAILED_FROM_SIMULATION) {
        _pendingFlags &= ~PENDING_FLAG_REMOVE_DETAILED_FROM_SIMULATION;
        for (auto& detailedMotionState : _detailedMotionStates) {
            transaction.objectsToRemove.push_back(detailedMotionState);
        }
        // NOTE: the DetailedMotionStates are deleted after being added to PhysicsEngine::Transaction::_objectsToRemove
        // See AvatarManager::handleProcessedPhysicsTransaction()
        _detailedMotionStates.clear();
    }
    if (_pendingFlags & PENDING_FLAG_ADD_DETAILED_TO_SIMULATION) {
        _pendingFlags &= ~PENDING_FLAG_ADD_DETAILED_TO_SIMULATION;
        for (int32_t i = 0; i < _avatar->getJointCount(); i++) {
            auto dMotionState = createDetailedMotionStateForJoint(i);
            if (dMotionState) {
                _detailedMotionStates.push_back(dMotionState);
                transaction.objectsToAdd.push_back(dMotionState);
            }
        }
    }
}

class DetailedRayResultCallback : public btCollisionWorld::AllHitsRayResultCallback {
public:
    DetailedRayResultCallback()
        : btCollisionWorld::AllHitsRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
        // the RayResultCallback's group and mask must match MY_AVATAR
        m_collisionFilterGroup = BULLET_COLLISION_GROUP_DETAILED_RAY;
        m_collisionFilterMask = BULLET_COLLISION_MASK_DETAILED_RAY;
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override {
        return AllHitsRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
};

std::vector<MyCharacterController::RayAvatarResult> MyCharacterController::rayTest(const btVector3& origin, const btVector3& direction,
                                                                                   const btScalar& length, const QVector<uint>& jointsToExclude) const {
    std::vector<RayAvatarResult> foundAvatars;
    if (_physicsEngine) {
        btVector3 end = origin + length * direction;
        DetailedRayResultCallback rayCallback = DetailedRayResultCallback();
        rayCallback.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
        rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseSubSimplexConvexCastRaytest;
        _physicsEngine->getDynamicsWorld()->rayTest(origin, end, rayCallback);
        if (rayCallback.m_hitFractions.size() > 0) {
            foundAvatars.reserve(rayCallback.m_hitFractions.size());
            for (int32_t i = 0; i < rayCallback.m_hitFractions.size(); i++) {
                auto object = rayCallback.m_collisionObjects[i];
                ObjectMotionState* motionState = static_cast<ObjectMotionState*>(object->getUserPointer());
                if (motionState && motionState->getType() == MOTIONSTATE_TYPE_DETAILED) {
                    DetailedMotionState* detailedMotionState = dynamic_cast<DetailedMotionState*>(motionState);
                    MyCharacterController::RayAvatarResult result;
                    result._intersect = true;
                    result._intersectWithAvatar = detailedMotionState->getAvatarID();
                    result._intersectionPoint = bulletToGLM(rayCallback.m_hitPointWorld[i]);
                    result._intersectionNormal = bulletToGLM(rayCallback.m_hitNormalWorld[i]);
                    result._distance = length * rayCallback.m_hitFractions[i];
                    result._intersectWithJoint = detailedMotionState->getJointIndex();
                    result._isBound = detailedMotionState->getIsBound(result._boundJoints);
                    btVector3 center;
                    btScalar radius;
                    detailedMotionState->getShape()->getBoundingSphere(center, radius);
                    result._maxDistance = (float)radius;
                    foundAvatars.push_back(result);
                }
            }
            std::sort(foundAvatars.begin(), foundAvatars.end(), [](const RayAvatarResult& resultA, const RayAvatarResult& resultB) {
                return resultA._distance < resultB._distance;
            });
        }
    }
    return foundAvatars;
}
