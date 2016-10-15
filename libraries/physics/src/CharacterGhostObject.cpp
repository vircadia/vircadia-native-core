//
//  CharacterGhostObject.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2016.08.26
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CharacterGhostObject.h"

#include <stdint.h>
#include <assert.h>

#include <PhysicsHelpers.h>

#include "CharacterRayResult.h"
#include "CharacterGhostShape.h"


CharacterGhostObject::~CharacterGhostObject() {
    removeFromWorld();
    if (_ghostShape) {
        delete _ghostShape;
        _ghostShape = nullptr;
        setCollisionShape(nullptr);
    }
}

void CharacterGhostObject::setCollisionGroupAndMask(int16_t group, int16_t mask) {
    _collisionFilterGroup = group;
    _collisionFilterMask = mask;
    // TODO: if this probe is in the world reset ghostObject overlap cache
}

void CharacterGhostObject::getCollisionGroupAndMask(int16_t& group, int16_t& mask) const {
    group = _collisionFilterGroup;
    mask = _collisionFilterMask;
}

void CharacterGhostObject::setRadiusAndHalfHeight(btScalar radius, btScalar halfHeight) {
    _radius = radius;
    _halfHeight = halfHeight;
}

void CharacterGhostObject::setUpDirection(const btVector3& up) {
    btScalar length = up.length();
    if (length > FLT_EPSILON) {
        _upDirection /= length;
    } else {
        _upDirection = btVector3(0.0f, 1.0f, 0.0f);
    }
}

void CharacterGhostObject::setMotorVelocity(const btVector3& velocity) {
    _motorVelocity = velocity;
    if (_motorOnly) {
        _linearVelocity = _motorVelocity;
    }
}

// override of btCollisionObject::setCollisionShape()
void CharacterGhostObject::setCharacterShape(btConvexHullShape* shape) {
    assert(shape);
    // we create our own shape with an expanded Aabb for more reliable sweep tests
    if (_ghostShape) {
        delete _ghostShape;
    }

    _ghostShape = new CharacterGhostShape(static_cast<const btConvexHullShape*>(shape));
    setCollisionShape(_ghostShape);
}

void CharacterGhostObject::setCollisionWorld(btCollisionWorld* world) {
    if (world != _world) {
        removeFromWorld();
        _world = world;
        addToWorld();
    }
}

void CharacterGhostObject::move(btScalar dt, btScalar overshoot, btScalar gravity) {
    bool oldOnFloor = _onFloor;
    _onFloor = false;
    _steppingUp = false;
    assert(_world && _inWorld);
    updateVelocity(dt, gravity);

    // resolve any penetrations before sweeping
    int32_t MAX_LOOPS = 4;
    int32_t numExtractions = 0;
    btVector3 totalPosition(0.0f, 0.0f, 0.0f);
    while (numExtractions < MAX_LOOPS) {
        if (resolvePenetration(numExtractions)) {
            numExtractions = 0;
            break;
        }
        totalPosition += getWorldTransform().getOrigin();
        ++numExtractions;
    }
    if (numExtractions > 1) {
        // penetration resolution was probably oscillating between opposing objects
        // so we use the average of the solutions
        totalPosition /= btScalar(numExtractions);
        btTransform transform = getWorldTransform();
        transform.setOrigin(totalPosition);
        setWorldTransform(transform);

        // TODO: figure out how to untrap character
    }
    btTransform startTransform = getWorldTransform();
    btVector3 startPosition = startTransform.getOrigin();
    if (_onFloor) {
        // resolvePenetration() pushed the avatar out of a floor so
        // we must updateTraction() before using _linearVelocity
        updateTraction(startPosition);
    }

    btScalar speed = _linearVelocity.length();
    btVector3 forwardSweep = dt * _linearVelocity;
    btScalar stepDistance = dt * speed;
    btScalar MIN_SWEEP_DISTANCE = 0.0001f;
    if (stepDistance < MIN_SWEEP_DISTANCE) {
        // not moving, no need to sweep
        updateTraction(startPosition);
        return;
    }

    // augment forwardSweep to help slow moving sweeps get over steppable ledges
    const btScalar MIN_OVERSHOOT = 0.04f; // default margin
    if (overshoot < MIN_OVERSHOOT) {
        overshoot = MIN_OVERSHOOT;
    }
    btScalar longSweepDistance = stepDistance + overshoot;
    forwardSweep *= longSweepDistance / stepDistance;

    // step forward
    CharacterSweepResult result(this);
    btTransform nextTransform = startTransform;
    nextTransform.setOrigin(startPosition + forwardSweep);
    sweepTest(_characterShape, startTransform, nextTransform, result); // forward

    if (!result.hasHit()) {
        nextTransform.setOrigin(startPosition + (stepDistance / longSweepDistance) * forwardSweep);
        setWorldTransform(nextTransform);
        updateTraction(nextTransform.getOrigin());
        return;
    }
    bool verticalOnly = btFabs(btFabs(_linearVelocity.dot(_upDirection)) - speed) < MIN_OVERSHOOT;
    if (verticalOnly) {
        // no need to step
        nextTransform.setOrigin(startPosition + (result.m_closestHitFraction * stepDistance / longSweepDistance) * forwardSweep);
        setWorldTransform(nextTransform);

        if (result.m_hitNormalWorld.dot(_upDirection) > _maxWallNormalUpComponent) {
            _floorNormal = result.m_hitNormalWorld;
            _floorContact = result.m_hitPointWorld;
            _steppingUp = false;
            _onFloor = true;
            _hovering = false;
        }
        updateTraction(nextTransform.getOrigin());
        return;
    }

    // check if this hit is obviously unsteppable
    btVector3 hitFromBase = result.m_hitPointWorld - (startPosition - ((_radius + _halfHeight) * _upDirection));
    btScalar hitHeight = hitFromBase.dot(_upDirection);
    if (hitHeight > _maxStepHeight) {
        // shape can't step over the obstacle so move forward as much as possible before we bail
        btVector3 forwardTranslation = result.m_closestHitFraction * forwardSweep;
        btScalar forwardDistance = forwardTranslation.length();
        if (forwardDistance > stepDistance) {
            forwardTranslation *= stepDistance / forwardDistance;
        }
        nextTransform.setOrigin(startPosition + forwardTranslation);
        setWorldTransform(nextTransform);
        _onFloor = _onFloor || oldOnFloor;
        return;
    }
    // if we get here then we hit something that might be steppable

    // remember the forward sweep hit fraction for later
    btScalar forwardSweepHitFraction = result.m_closestHitFraction;

    // figure out how high we can step up
    btScalar availableStepHeight = measureAvailableStepHeight();

    // raise by availableStepHeight before sweeping forward
    result.resetHitHistory();
    startTransform.setOrigin(startPosition + availableStepHeight * _upDirection);
    nextTransform.setOrigin(startTransform.getOrigin() + forwardSweep);
    sweepTest(_characterShape, startTransform, nextTransform, result);
    if (result.hasHit()) {
        startTransform.setOrigin(startTransform.getOrigin() + result.m_closestHitFraction * forwardSweep);
    } else {
        startTransform = nextTransform;
    }

    // sweep down in search of future landing spot
    result.resetHitHistory();
    btVector3 downSweep = (- availableStepHeight) * _upDirection;
    nextTransform.setOrigin(startTransform.getOrigin() + downSweep);
    sweepTest(_characterShape, startTransform, nextTransform, result);
    if (result.hasHit() && result.m_hitNormalWorld.dot(_upDirection) > _maxWallNormalUpComponent) {
        // can stand on future landing spot, so we interpolate toward it
        _floorNormal = result.m_hitNormalWorld;
        _floorContact = result.m_hitPointWorld;
        _steppingUp = true;
        _onFloor = true;
        _hovering = false;
        nextTransform.setOrigin(startTransform.getOrigin() + result.m_closestHitFraction * downSweep);
        btVector3 totalStep = nextTransform.getOrigin() - startPosition;
        nextTransform.setOrigin(startPosition + (stepDistance / totalStep.length()) * totalStep);
        updateTraction(nextTransform.getOrigin());
    } else {
        // either there is no future landing spot, or there is but we can't stand on it
        // in any case: we go forward as much as possible
        nextTransform.setOrigin(startPosition + forwardSweepHitFraction * (stepDistance / longSweepDistance) * forwardSweep);
        _onFloor = _onFloor || oldOnFloor;
        updateTraction(nextTransform.getOrigin());
    }
    setWorldTransform(nextTransform);
}

bool CharacterGhostObject::sweepTest(
        const btConvexShape* shape,
        const btTransform& start,
        const btTransform& end,
        CharacterSweepResult& result) const {
    if (_world && _inWorld) {
        assert(shape);
        btScalar allowedPenetration = _world->getDispatchInfo().m_allowedCcdPenetration;
        convexSweepTest(shape, start, end, result, allowedPenetration);
        return result.hasHit();
    }
    return false;
}

void CharacterGhostObject::measurePenetration(btVector3& minBoxOut, btVector3& maxBoxOut) {
    // minBoxOut and maxBoxOut will be updated with penetration envelope.
    // If one of the corner points is <0,0,0> then the penetration is resolvable in a single step,
    // but if the space spanned by the two corners extends in both directions along at least one
    // component then we the object is sandwiched between two opposing objects.

    // We assume this object has just been moved to its current location, or else objects have been
    // moved around it since the last step so we must update the overlapping pairs.
    refreshOverlappingPairCache();

    // compute collision details
    btHashedOverlappingPairCache* pairCache = getOverlappingPairCache();
    _world->getDispatcher()->dispatchAllCollisionPairs(pairCache, _world->getDispatchInfo(), _world->getDispatcher());

    // loop over contact manifolds to compute the penetration box
    minBoxOut = btVector3(0.0f, 0.0f, 0.0f);
    maxBoxOut = btVector3(0.0f, 0.0f, 0.0f);
    btManifoldArray manifoldArray;

    int numPairs = pairCache->getNumOverlappingPairs();
    for (int i = 0; i < numPairs; i++) {
        manifoldArray.resize(0);
        btBroadphasePair* collisionPair = &(pairCache->getOverlappingPairArray()[i]);

        btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
        btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

        if ((obj0 && !obj0->hasContactResponse()) && (obj1 && !obj1->hasContactResponse())) {
            // we know this probe has no contact response
            // but neither does the other object so skip this manifold
            continue;
        }

        if (!collisionPair->m_algorithm) {
            // null m_algorithm means the two shape types don't know how to collide!
            // shouldn't fall in here but just in case
            continue;
        }

        btScalar mostFloorPenetration = 0.0f;
        collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);
        for (int j = 0;j < manifoldArray.size(); j++) {
            btPersistentManifold* manifold = manifoldArray[j];
            btScalar directionSign = (manifold->getBody0() == this) ? btScalar(1.0) : btScalar(-1.0);
            for (int p = 0; p < manifold->getNumContacts(); p++) {
                const btManifoldPoint& pt = manifold->getContactPoint(p);
                if (pt.getDistance() > 0.0f) {
                    continue;
                }

                // normal always points from object to character
                btVector3 normal = directionSign * pt.m_normalWorldOnB;

                btScalar penetrationDepth = pt.getDistance();
                if (penetrationDepth < mostFloorPenetration) { // remember penetrationDepth is negative
                    btScalar normalDotUp = normal.dot(_upDirection);
                    if (normalDotUp > _maxWallNormalUpComponent) {
                        mostFloorPenetration = penetrationDepth;
                        _floorNormal = normal;
                        if (directionSign > 0.0f) {
                            _floorContact = pt.m_positionWorldOnA;
                        } else {
                            _floorContact = pt.m_positionWorldOnB;
                        }
                        _onFloor = true;
                    }
                }

                btVector3 penetration = (-penetrationDepth) * normal;
                minBoxOut.setMin(penetration);
                maxBoxOut.setMax(penetration);
            }
        }
    }
}

void CharacterGhostObject::removeFromWorld() {
    if (_world && _inWorld) {
        _world->removeCollisionObject(this);
        _inWorld = false;
    }
}

void CharacterGhostObject::addToWorld() {
    if (_world && !_inWorld) {
        assert(getCollisionShape());
        setCollisionFlags(getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        _world->addCollisionObject(this, _collisionFilterGroup, _collisionFilterMask);
        _inWorld = true;
    }
}

bool CharacterGhostObject::rayTest(const btVector3& start,
        const btVector3& end,
        CharacterRayResult& result) const {
    if (_world && _inWorld) {
        _world->rayTest(start, end, result);
    }
    return result.hasHit();
}

bool CharacterGhostObject::resolvePenetration(int numTries) {
    btVector3 minBox, maxBox;
    measurePenetration(minBox, maxBox);
    btVector3 restore = maxBox + minBox;
    if (restore.length2() > 0.0f) {
        btTransform transform = getWorldTransform();
        transform.setOrigin(transform.getOrigin() + restore);
        setWorldTransform(transform);
        return false;
    }
    return true;
}

void CharacterGhostObject::refreshOverlappingPairCache() {
    assert(_world && _inWorld);
    btVector3 minAabb, maxAabb;
    getCollisionShape()->getAabb(getWorldTransform(), minAabb, maxAabb);
    // this updates both pairCaches: world broadphase and ghostobject
    _world->getBroadphase()->setAabb(getBroadphaseHandle(), minAabb, maxAabb, _world->getDispatcher());
}

void CharacterGhostObject::updateVelocity(btScalar dt, btScalar gravity) {
    if (!_motorOnly) {
        if (_hovering) {
            _linearVelocity *= 0.999f; // HACK damping
        } else {
            _linearVelocity += (dt * gravity) * _upDirection;
        }
    }
}

void CharacterGhostObject::updateHoverState(const btVector3& position) {
    if (_onFloor) {
        _hovering = false;
    } else {
        // cast a ray down looking for floor support
        CharacterRayResult rayResult(this);
        btScalar distanceToFeet = _radius + _halfHeight;
        btScalar slop = 2.0f * getCollisionShape()->getMargin(); // slop to help ray start OUTSIDE the floor object
        btVector3 startPos = position - ((distanceToFeet - slop) * _upDirection);
        btVector3 endPos = startPos - (2.0f * distanceToFeet) * _upDirection;
        rayTest(startPos, endPos, rayResult);
        // we're hovering if the ray didn't hit anything or hit unstandable slope
        _hovering = !rayResult.hasHit() || rayResult.m_hitNormalWorld.dot(_upDirection) < _maxWallNormalUpComponent;
    }
}

void CharacterGhostObject::updateTraction(const btVector3& position) {
    updateHoverState(position);
    if (_hovering || _motorOnly) {
        _linearVelocity = _motorVelocity;
    } else if (_onFloor) {
        // compute a velocity that swings the shape around the _floorContact
        btVector3 leverArm = _floorContact - position;
        btVector3 pathDirection = leverArm.cross(_motorVelocity.cross(leverArm));
        btScalar pathLength = pathDirection.length();
        if (pathLength > FLT_EPSILON) {
            _linearVelocity = (_motorVelocity.length() / pathLength) * pathDirection;
        } else {
            _linearVelocity = btVector3(0.0f, 0.0f, 0.0f);
        }
    }
}

btScalar CharacterGhostObject::measureAvailableStepHeight() const {
    CharacterSweepResult result(this);
    btTransform transform = getWorldTransform();
    btTransform nextTransform = transform;
    nextTransform.setOrigin(transform.getOrigin() + _maxStepHeight * _upDirection);
    sweepTest(_characterShape, transform, nextTransform, result);
    return result.m_closestHitFraction * _maxStepHeight;
}

