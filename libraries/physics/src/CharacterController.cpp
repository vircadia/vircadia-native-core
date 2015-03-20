/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2008 Erwin Coumans  http://bulletphysics.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
   If you use this software in a product, an acknowledgment in the product documentation would be appreciated 
   but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include "BulletCollision/CollisionDispatch/btGhostObject.h"

#include "BulletUtil.h"
#include "CharacterController.h"


// static helper method
static btVector3 getNormalizedVector(const btVector3& v) {
    // NOTE: check the length first, then normalize 
    // --> avoids assert when trying to normalize zero-length vectors
    btScalar vLength = v.length();
    if (vLength < FLT_EPSILON) {
        return btVector3(0.0f, 0.0f, 0.0f);
    }
    btVector3 n = v;
    n /= vLength;
    return n;
}

///@todo Interact with dynamic objects,
///Ride kinematicly animated platforms properly
///More realistic (or maybe just a config option) falling
/// -> Should integrate falling velocity manually and use that in stepDown()
///Support jumping
///Support ducking

/* This callback is unused, but we're keeping it around just in case we figure out how to use it.
class btKinematicClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
btKinematicClosestNotMeRayResultCallback(btCollisionObject* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
{
m_me = me;
}

virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
{
if (rayResult.m_collisionObject == m_me)
return 1.0;

return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
}
protected:
btCollisionObject* m_me;
};
*/

class btKinematicClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
    public:
        btKinematicClosestNotMeConvexResultCallback(btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
            : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
              , m_me(me)
              , m_up(up)
              , m_minSlopeDot(minSlopeDot)
    {
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) {
        if (convexResult.m_hitCollisionObject == m_me) {
            return btScalar(1.0);
        }

        if (!convexResult.m_hitCollisionObject->hasContactResponse()) {
            return btScalar(1.0);
        }

        btVector3 hitNormalWorld;
        if (normalInWorldSpace) {
            hitNormalWorld = convexResult.m_hitNormalLocal;
        } else {
            ///need to transform normal into worldspace
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }

        // Note: hitNormalWorld points into character, away from object
        // and m_up points opposite to movement

        btScalar dotUp = m_up.dot(hitNormalWorld);
        if (dotUp < m_minSlopeDot) {
            return btScalar(1.0);
        }

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

protected:
    btCollisionObject* m_me;
    const btVector3 m_up;
    btScalar m_minSlopeDot;
};

class StepDownConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
    // special convex sweep callback for character during the stepDown() phase
    public:
        StepDownConvexResultCallback(btCollisionObject* me, 
                const btVector3& up, 
                const btVector3& start,
                const btVector3& step,
                const btVector3& pushDirection,
                btScalar minSlopeDot,
                btScalar radius,
                btScalar halfHeight)
            : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
              , m_me(me)
              , m_up(up)
              , m_start(start)
              , m_step(step)
              , m_pushDirection(pushDirection)
              , m_minSlopeDot(minSlopeDot)
              , m_radius(radius)
              , m_halfHeight(halfHeight)
    {
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) {
        if (convexResult.m_hitCollisionObject == m_me) {
            return btScalar(1.0);
        }

        if (!convexResult.m_hitCollisionObject->hasContactResponse()) {
            return btScalar(1.0);
        }

        btVector3 hitNormalWorld;
        if (normalInWorldSpace) {
            hitNormalWorld = convexResult.m_hitNormalLocal;
        } else {
            ///need to transform normal into worldspace
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
        }

        // Note: hitNormalWorld points into character, away from object
        // and m_up points opposite to movement

        btScalar dotUp = m_up.dot(hitNormalWorld);
        if (dotUp < m_minSlopeDot) {
            if (hitNormalWorld.dot(m_pushDirection) > 0.0f) {
                // ignore hits that push in same direction as character is moving
                // which helps character NOT snag when stepping off ledges
                return btScalar(1.0f);
            }

            // compute the angle between "down" and the line from character center to "hit" point
            btVector3 fractionalStep = convexResult.m_hitFraction * m_step;
            btVector3 localHit = convexResult.m_hitPointLocal - m_start + fractionalStep;
            btScalar angle = localHit.angle(-m_up);

            // compute a maxAngle based on size of m_step
            btVector3 side(m_radius, - (m_halfHeight - m_step.length() + fractionalStep.dot(m_up)), 0.0f);
            btScalar maxAngle = side.angle(-m_up);

            // Ignore hits that are larger than maxAngle. Effectively what is happening here is: 
            // we're ignoring hits at contacts that have non-vertical normals... if they hit higher
            // than the character's "feet".  Ignoring the contact allows the character to slide down
            // for these hits.  In other words, vertical walls against the character's torso will
            // not prevent them from "stepping down" to find the floor.
            if (angle > maxAngle) {
                return btScalar(1.0f);
            }
        }

        btScalar fraction = ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
        return fraction;
    }

protected:
    btCollisionObject* m_me;
    const btVector3 m_up;
    btVector3 m_start;
    btVector3 m_step;
    btVector3 m_pushDirection;
    btScalar m_minSlopeDot;
    btScalar m_radius;
    btScalar m_halfHeight;
};

/*
 * Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
 *
 * from: http://www-cs-students.stanford.edu/~adityagp/final/node3.html
 */
btVector3 CharacterController::computeReflectionDirection(const btVector3& direction, const btVector3& normal) {
    return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
}

/*
 * Returns the portion of 'direction' that is parallel to 'normal'
 */
btVector3 CharacterController::parallelComponent(const btVector3& direction, const btVector3& normal) {
    btScalar magnitude = direction.dot(normal);
    return normal * magnitude;
}

/*
 * Returns the portion of 'direction' that is perpindicular to 'normal'
 */
btVector3 CharacterController::perpindicularComponent(const btVector3& direction, const btVector3& normal) {
    return direction - parallelComponent(direction, normal);
}

CharacterController::CharacterController(AvatarData* avatarData) {
    assert(avatarData);
    m_avatarData = avatarData;

    // cache the "PhysicsEnabled" state of m_avatarData
    m_avatarData->lockForRead();
    m_enabled = m_avatarData->isPhysicsEnabled();
    m_avatarData->unlock();

    createShapeAndGhost();

    m_upAxis = 1; // HACK: hard coded to be 1 for now (yAxis)

    m_addedMargin = 0.02f;
    m_walkDirection.setValue(0.0f,0.0f,0.0f);
    m_turnAngle = btScalar(0.0f);
    m_useWalkDirection = true; // use walk direction by default, legacy behavior
    m_velocityTimeInterval = 0.0f;
    m_verticalVelocity = 0.0f;
    m_verticalOffset = 0.0f;
    m_gravity = 9.8f;
    m_maxFallSpeed = 55.0f; // Terminal velocity of a sky diver in m/s.
    m_jumpSpeed = 10.0f; // ?
    m_wasOnGround = false;
    m_wasJumping = false;
    m_interpolateUp = true;
    setMaxSlope(btRadians(45.0f));
    m_lastStepUp = 0.0f;

    // internal state data members
    full_drop = false;
    bounce_fix = false;
}

CharacterController::~CharacterController() {
}

btPairCachingGhostObject* CharacterController::getGhostObject() {
    return m_ghostObject;
}

bool CharacterController::recoverFromPenetration(btCollisionWorld* collisionWorld) {
    // Here we must refresh the overlapping paircache as the penetrating movement itself or the
    // previous recovery iteration might have used setWorldTransform and pushed us into an object
    // that is not in the previous cache contents from the last timestep, as will happen if we
    // are pushed into a new AABB overlap. Unhandled this means the next convex sweep gets stuck.
    //
    // Do this by calling the broadphase's setAabb with the moved AABB, this will update the broadphase
    // paircache and the ghostobject's internal paircache at the same time.    /BW

    btVector3 minAabb, maxAabb;
    m_convexShape->getAabb(m_ghostObject->getWorldTransform(), minAabb, maxAabb);
    collisionWorld->getBroadphase()->setAabb(m_ghostObject->getBroadphaseHandle(), 
            minAabb, 
            maxAabb, 
            collisionWorld->getDispatcher());

    bool penetration = false;

    collisionWorld->getDispatcher()->dispatchAllCollisionPairs(m_ghostObject->getOverlappingPairCache(), collisionWorld->getDispatchInfo(), collisionWorld->getDispatcher());

    m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
    btVector3 up = getUpAxisDirections()[m_upAxis];

    btVector3 currentPosition = m_currentPosition;

    btScalar maxPen = btScalar(0.0);
    for (int i = 0; i < m_ghostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++) {
        m_manifoldArray.resize(0);

        btBroadphasePair* collisionPair = &m_ghostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

        btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
        btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

        if ((obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse())) {
            continue;
        }

        if (collisionPair->m_algorithm) {
            collisionPair->m_algorithm->getAllContactManifolds(m_manifoldArray);
        }

        for (int j = 0;j < m_manifoldArray.size(); j++) {
            btPersistentManifold* manifold = m_manifoldArray[j];
            btScalar directionSign = (manifold->getBody0() == m_ghostObject) ? btScalar(1.0) : btScalar(-1.0);
            for (int p = 0;p < manifold->getNumContacts(); p++) {
                const btManifoldPoint&pt = manifold->getContactPoint(p);

                btScalar dist = pt.getDistance();

                if (dist < 0.0) {
                    bool useContact = true;
                    btVector3 normal = pt.m_normalWorldOnB;
                    normal *= directionSign; // always points from object to character

                    btScalar normalDotUp = normal.dot(up);
                    if (normalDotUp < m_maxSlopeCosine) {
                        // this contact has a non-vertical normal... might need to ignored
                        btVector3 collisionPoint;
                        if (directionSign > 0.0) {
                            collisionPoint = pt.getPositionWorldOnB();
                        } else {
                            collisionPoint = pt.getPositionWorldOnA();
                        }

                        // we do math in frame where character base is origin
                        btVector3 characterBase = currentPosition - (m_radius + m_halfHeight) * up;
                        collisionPoint -= characterBase;
                        btScalar collisionHeight = collisionPoint.dot(up);

                        if (collisionHeight < m_lastStepUp) {
                            // This contact is below the lastStepUp, so we ignore it for penetration resolution,
                            // otherwise it may prevent the character from getting close enough to find any available 
                            // horizontal foothold that would allow it to climbe the ledge.  In other words, we're
                            // making the character's "feet" soft for collisions against steps, but not floors.
                            useContact = false;
                        } 
                    }
                    if (useContact) {
    
                        if (dist < maxPen) {
                            maxPen = dist;
                            m_floorNormal = normal;
                        }
                        const btScalar INCREMENTAL_RESOLUTION_FACTOR = 0.2f;
                        m_currentPosition += normal * (fabsf(dist) * INCREMENTAL_RESOLUTION_FACTOR);
                        penetration = true;
                    }
                }
            }
        }
    }
    btTransform newTrans = m_ghostObject->getWorldTransform();
    newTrans.setOrigin(m_currentPosition);
    m_ghostObject->setWorldTransform(newTrans);
    return penetration;
}

void CharacterController::stepUp( btCollisionWorld* world) {
    // phase 1: up

    // compute start and end
    btTransform start, end;
    start.setIdentity();
    start.setOrigin(m_currentPosition + getUpAxisDirections()[m_upAxis] * (m_convexShape->getMargin() + m_addedMargin));

    //m_targetPosition = m_currentPosition + getUpAxisDirections()[m_upAxis] * (m_stepHeight + (m_verticalOffset > 0.0f ? m_verticalOffset : 0.0f));
    m_targetPosition = m_currentPosition + getUpAxisDirections()[m_upAxis] * m_stepHeight;
    end.setIdentity();
    end.setOrigin(m_targetPosition);

    // sweep up
    btVector3 sweepDirNegative = -getUpAxisDirections()[m_upAxis];
    btKinematicClosestNotMeConvexResultCallback callback(m_ghostObject, sweepDirNegative, btScalar(0.7071));
    callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;
    m_ghostObject->convexSweepTest(m_convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

    if (callback.hasHit()) {
        // we hit something, so zero our vertical velocity
        m_verticalVelocity = 0.0;
        m_verticalOffset = 0.0;

        // Only modify the position if the hit was a slope and not a wall or ceiling.
        if (callback.m_hitNormalWorld.dot(getUpAxisDirections()[m_upAxis]) > 0.0) {
            m_lastStepUp = m_stepHeight * callback.m_closestHitFraction;
            if (m_interpolateUp == true) {
                m_currentPosition.setInterpolate3 (m_currentPosition, m_targetPosition, callback.m_closestHitFraction);
            } else {
                m_currentPosition = m_targetPosition;
            }
        } else {
            m_lastStepUp = m_stepHeight;
            m_currentPosition = m_targetPosition;
        }
    } else {
        m_currentPosition = m_targetPosition;
        m_lastStepUp = m_stepHeight;
    }
}

void CharacterController::updateTargetPositionBasedOnCollision(const btVector3& hitNormal, btScalar tangentMag, btScalar normalMag) {
    btVector3 movementDirection = m_targetPosition - m_currentPosition;
    btScalar movementLength = movementDirection.length();
    if (movementLength > SIMD_EPSILON) {
        movementDirection.normalize();

        btVector3 reflectDir = computeReflectionDirection(movementDirection, hitNormal);
        reflectDir.normalize();

        btVector3 parallelDir, perpindicularDir;

        parallelDir = parallelComponent(reflectDir, hitNormal);
        perpindicularDir = perpindicularComponent(reflectDir, hitNormal);

        m_targetPosition = m_currentPosition;
        //if (tangentMag != 0.0) {
        if (0) {
            btVector3 parComponent = parallelDir * btScalar(tangentMag * movementLength);
            m_targetPosition +=  parComponent;
        }

        if (normalMag != 0.0) {
            btVector3 perpComponent = perpindicularDir * btScalar(normalMag * movementLength);
            m_targetPosition += perpComponent;
        }
    }
}

void CharacterController::stepForward( btCollisionWorld* collisionWorld, const btVector3& movement) {
    // phase 2: forward
    m_targetPosition = m_currentPosition + movement;

    btTransform start, end;
    start.setIdentity();
    end.setIdentity();

    /* TODO: experiment with this to see if we can use this to help direct motion when a floor is available
    if (m_touchingContact) {
        if (m_normalizedDirection.dot(m_floorNormal) < btScalar(0.0)) {
            updateTargetPositionBasedOnCollision(m_floorNormal, 1.0f, 1.0f);
        }
    }*/

    // modify shape's margin for the sweeps
    btScalar margin = m_convexShape->getMargin();
    m_convexShape->setMargin(margin + m_addedMargin);

    const btScalar MIN_STEP_DISTANCE = 0.0001f;
    btVector3 step = m_targetPosition - m_currentPosition;
    btScalar stepLength2 = step.length2();
    int maxIter = 10;

    while (stepLength2 > MIN_STEP_DISTANCE && maxIter-- > 0) {
        start.setOrigin(m_currentPosition);
        end.setOrigin(m_targetPosition);

        // sweep forward
        btVector3 sweepDirNegative(m_currentPosition - m_targetPosition);
        btKinematicClosestNotMeConvexResultCallback callback(m_ghostObject, sweepDirNegative, btScalar(0.0));
        callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
        callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;
        m_ghostObject->convexSweepTest(m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);

        if (callback.hasHit()) {
            // we hit soemthing!
            // Compute new target position by removing portion cut-off by collision, which will produce a new target
            // that is the closest approach of the the obstacle plane to the original target.
            step = m_targetPosition - m_currentPosition;
            btScalar stepDotNormal = step.dot(callback.m_hitNormalWorld); // we expect this dot to be negative
            step += (stepDotNormal * (1.0f - callback.m_closestHitFraction)) * callback.m_hitNormalWorld;
            m_targetPosition = m_currentPosition + step;

            stepLength2 = step.length2();
        } else {
            // we swept to the end without hitting anything
            m_currentPosition = m_targetPosition;
            break;
        }
    }

    // restore shape's margin
    m_convexShape->setMargin(margin);
}

void CharacterController::stepDown( btCollisionWorld* collisionWorld, btScalar dt) {
    // phase 3: down
    //
    // The "stepDown" phase first makes a normal sweep down that cancels the lift from the "stepUp" phase.  
    // If it hits a ledge then it stops otherwise it makes another sweep down in search of a floor within
    // reach of the character's feet.

    btScalar downSpeed = (m_verticalVelocity < 0.0f) ? -m_verticalVelocity : 0.0f;
    if (downSpeed > 0.0f && downSpeed > m_maxFallSpeed && (m_wasOnGround || !m_wasJumping)) {
        downSpeed = m_maxFallSpeed;
    }

    // first sweep for ledge
    btVector3 step = getUpAxisDirections()[m_upAxis] * (-(m_lastStepUp + downSpeed * dt));

    StepDownConvexResultCallback callback(m_ghostObject, 
            getUpAxisDirections()[m_upAxis], 
            m_currentPosition, step, 
            m_walkDirection,
            m_maxSlopeCosine, 
            m_radius, m_halfHeight);
    callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

    btTransform start, end;
    start.setIdentity();
    end.setIdentity();

    start.setOrigin(m_currentPosition);
    m_targetPosition = m_currentPosition + step;
    end.setOrigin(m_targetPosition);
    m_ghostObject->convexSweepTest(m_convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);

    if (callback.hasHit()) {
        m_currentPosition += callback.m_closestHitFraction * step;
        m_verticalVelocity = 0.0f;
        m_verticalOffset = 0.0f;
        m_wasJumping = false;
    } else {
        // sweep again for floor within downStep threshold
        StepDownConvexResultCallback callback2 (m_ghostObject, 
                getUpAxisDirections()[m_upAxis], 
                m_currentPosition, step,
                m_walkDirection,
                m_maxSlopeCosine, 
                m_radius, m_halfHeight);

        callback2.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
        callback2.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

        m_currentPosition = m_targetPosition;
        btVector3 oldPosition = m_currentPosition;
        step = (- m_stepHeight) * getUpAxisDirections()[m_upAxis];
        m_targetPosition = m_currentPosition + step;

        start.setOrigin(m_currentPosition);
        end.setOrigin(m_targetPosition);
        m_ghostObject->convexSweepTest(m_convexShape, start, end, callback2, collisionWorld->getDispatchInfo().m_allowedCcdPenetration);

        if (callback2.hasHit()) {
            m_currentPosition += callback2.m_closestHitFraction * step;
            m_verticalVelocity = 0.0f;
            m_verticalOffset = 0.0f;
            m_wasJumping = false;
        } else {
            // nothing to step down on, so remove the stepUp effect
            m_currentPosition = oldPosition - m_lastStepUp * getUpAxisDirections()[m_upAxis];
            m_lastStepUp = 0.0f;
        }
    }
}

void CharacterController::setWalkDirection(const btVector3& walkDirection) {
    m_useWalkDirection = true;
    m_walkDirection = walkDirection;
    m_normalizedDirection = getNormalizedVector(m_walkDirection);
}

void CharacterController::setVelocityForTimeInterval(const btVector3& velocity, btScalar timeInterval) {
    m_useWalkDirection = false;
    m_walkDirection = velocity;
    m_normalizedDirection = getNormalizedVector(m_walkDirection);
    m_velocityTimeInterval += timeInterval;
}

void CharacterController::reset( btCollisionWorld* collisionWorld ) {
    m_verticalVelocity = 0.0;
    m_verticalOffset = 0.0;
    m_wasOnGround = false;
    m_wasJumping = false;
    m_walkDirection.setValue(0,0,0);
    m_velocityTimeInterval = 0.0;

    //clear pair cache
    btHashedOverlappingPairCache *cache = m_ghostObject->getOverlappingPairCache();
    while (cache->getOverlappingPairArray().size() > 0) {
        cache->removeOverlappingPair(cache->getOverlappingPairArray()[0].m_pProxy0, cache->getOverlappingPairArray()[0].m_pProxy1, collisionWorld->getDispatcher());
    }
}

void CharacterController::warp(const btVector3& origin) {
    btTransform xform;
    xform.setIdentity();
    xform.setOrigin(origin);
    m_ghostObject->setWorldTransform(xform);
}


void CharacterController::preStep(  btCollisionWorld* collisionWorld) {
    if (!m_enabled) {
        return;
    }
    int numPenetrationLoops = 0;
    m_touchingContact = false;
    while (recoverFromPenetration(collisionWorld)) {
        numPenetrationLoops++;
        m_touchingContact = true;
        if (numPenetrationLoops > 4) {
            break;
        }
    }

    m_currentPosition = m_ghostObject->getWorldTransform().getOrigin();
    m_targetPosition = m_currentPosition;
}

void CharacterController::playerStep(  btCollisionWorld* collisionWorld, btScalar dt) {
    if (!m_enabled || (!m_useWalkDirection && m_velocityTimeInterval <= 0.0)) {
        return; // no motion
    }

    m_wasOnGround = onGround();

    // Update fall velocity.
    m_verticalVelocity -= m_gravity * dt;
    if (m_verticalVelocity > m_jumpSpeed) {
        m_verticalVelocity = m_jumpSpeed;
    } else if (m_verticalVelocity < -m_maxFallSpeed) {
        m_verticalVelocity = -m_maxFallSpeed;
    }
    m_verticalOffset = m_verticalVelocity * dt;

    btTransform xform;
    xform = m_ghostObject->getWorldTransform();

    // the algorithm is as follows:
    // (1) step the character up a little bit so that its forward step doesn't hit the floor
    // (2) step the character forward
    // (3) step the character down looking for new ledges, the original floor, or a floor one step below where we started

    stepUp(collisionWorld);
    if (m_useWalkDirection) {
        stepForward(collisionWorld, m_walkDirection);
    } else {
        // compute substep and decrement total interval
        btScalar dtMoving = (dt < m_velocityTimeInterval) ? dt : m_velocityTimeInterval;
        m_velocityTimeInterval -= dt;

        // stepForward substep
        btVector3 move = m_walkDirection * dtMoving;
        stepForward(collisionWorld, move);
    }
    stepDown(collisionWorld, dt);

    xform.setOrigin(m_currentPosition);
    m_ghostObject->setWorldTransform(xform);
}

void CharacterController::setMaxFallSpeed(btScalar speed) {
    m_maxFallSpeed = speed;
}

void CharacterController::setJumpSpeed(btScalar jumpSpeed) {
    m_jumpSpeed = jumpSpeed;
}

void CharacterController::setMaxJumpHeight(btScalar maxJumpHeight) {
    m_maxJumpHeight = maxJumpHeight;
}

bool CharacterController::canJump() const {
    return onGround();
}

void CharacterController::jump() {
    if (!canJump()) {
        return;
    }

    m_verticalVelocity = m_jumpSpeed;
    m_wasJumping = true;

#if 0
    currently no jumping.
    btTransform xform;
    m_rigidBody->getMotionState()->getWorldTransform(xform);
    btVector3 up = xform.getBasis()[1];
    up.normalize();
    btScalar magnitude = (btScalar(1.0)/m_rigidBody->getInvMass()) * btScalar(8.0);
    m_rigidBody->applyCentralImpulse (up * magnitude);
#endif
}

void CharacterController::setGravity(btScalar gravity) {
    m_gravity = gravity;
}

btScalar CharacterController::getGravity() const {
    return m_gravity;
}

void CharacterController::setMaxSlope(btScalar slopeRadians) {
    m_maxSlopeRadians = slopeRadians;
    m_maxSlopeCosine = btCos(slopeRadians);
}

btScalar CharacterController::getMaxSlope() const {
    return m_maxSlopeRadians;
}

bool CharacterController::onGround() const {
    return m_enabled && m_verticalVelocity == 0.0 && m_verticalOffset == 0.0;
}

btVector3* CharacterController::getUpAxisDirections() {
    static btVector3 sUpAxisDirection[3] = { btVector3(1.0f, 0.0f, 0.0f), btVector3(0.0f, 1.0f, 0.0f), btVector3(0.0f, 0.0f, 1.0f) };

    return sUpAxisDirection;
}

void CharacterController::debugDraw(btIDebugDraw* debugDrawer) {
}

void CharacterController::setUpInterpolate(bool value) {
    m_interpolateUp = value;
}

// protected
void CharacterController::createShapeAndGhost() {
    // get new dimensions from avatar
    m_avatarData->lockForRead();
    AABox box = m_avatarData->getLocalAABox();

    // create new ghost
    m_ghostObject = new btPairCachingGhostObject();
    m_ghostObject->setWorldTransform(btTransform(glmToBullet(m_avatarData->getOrientation()),
                                                    glmToBullet(m_avatarData->getPosition())));
    m_avatarData->unlock();

    const glm::vec3& diagonal = box.getScale();
    m_radius = 0.5f * sqrtf(0.5f * (diagonal.x * diagonal.x + diagonal.z * diagonal.z));
    m_halfHeight = 0.5f * diagonal.y - m_radius;
    float MIN_HALF_HEIGHT = 0.1f;
    if (m_halfHeight < MIN_HALF_HEIGHT) {
        m_halfHeight = MIN_HALF_HEIGHT;
    }
    glm::vec3 offset = box.getCorner() + 0.5f * diagonal;
    m_shapeLocalOffset = offset;

    // stepHeight affects the heights of ledges that the character can ascend
    // however the actual ledge height is some function of m_stepHeight 
    // due to character shape and this CharacterController algorithm 
    // (the function is approximately 2*m_stepHeight)
    m_stepHeight = 0.1f * (m_radius + m_halfHeight) + 0.1f;

    // create new shape
    m_convexShape = new btCapsuleShape(m_radius, 2.0f * m_halfHeight);
    m_ghostObject->setCollisionShape(m_convexShape);
    m_ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
}

bool CharacterController::needsShapeUpdate() {
    // get new dimensions from avatar
    m_avatarData->lockForRead();
    AABox box = m_avatarData->getLocalAABox();
    m_avatarData->unlock();

    const glm::vec3& diagonal = box.getScale();
    float radius = 0.5f * sqrtf(0.5f * (diagonal.x * diagonal.x + diagonal.z * diagonal.z));
    float halfHeight = 0.5f * diagonal.y - radius;
    float MIN_HALF_HEIGHT = 0.1f;
    if (halfHeight < MIN_HALF_HEIGHT) {
        halfHeight = MIN_HALF_HEIGHT;
    }
    glm::vec3 offset = box.getCorner() + 0.5f * diagonal;

    // compare dimensions (and offset)
    float radiusDelta = glm::abs(radius - m_radius);
    float heightDelta = glm::abs(halfHeight - m_halfHeight);
    if (radiusDelta < FLT_EPSILON && heightDelta < FLT_EPSILON) {
        // shape hasn't changed --> nothing to do
        float offsetDelta = glm::distance(offset, m_shapeLocalOffset);
        if (offsetDelta > FLT_EPSILON) {
            // if only the offset changes then we can update it --> no need to rebuild shape
            m_shapeLocalOffset = offset;
        }
        return false;
    }
    return true;
}

void CharacterController::updateShape() {
    // DANGER: make sure this CharacterController and its GhostShape have been removed from
    // the PhysicsEngine before calling this.

    // delete shape and GhostObject
    delete m_ghostObject;
    m_ghostObject = NULL;
    delete m_convexShape;
    m_convexShape = NULL;
   
    createShapeAndGhost();
}

void CharacterController::preSimulation(btScalar timeStep) {
    m_avatarData->lockForRead();

    // cache the "PhysicsEnabled" state of m_avatarData here 
    // and use the cached value for the rest of the simulation step
    m_enabled = m_avatarData->isPhysicsEnabled();

    glm::quat rotation = m_avatarData->getOrientation();
    glm::vec3 position = m_avatarData->getPosition() + rotation * m_shapeLocalOffset;
    m_ghostObject->setWorldTransform(btTransform(glmToBullet(rotation), glmToBullet(position)));
    btVector3 walkVelocity = glmToBullet(m_avatarData->getVelocity());
    setVelocityForTimeInterval(walkVelocity, timeStep);

    m_avatarData->unlock();
}

void CharacterController::postSimulation() {
    if (m_enabled) {
        m_avatarData->lockForWrite();
        const btTransform& avatarTransform = m_ghostObject->getWorldTransform();            
        glm::quat rotation = bulletToGLM(avatarTransform.getRotation());
        glm::vec3 offset = rotation * m_shapeLocalOffset;
        m_avatarData->setOrientation(rotation);
        m_avatarData->setPosition(bulletToGLM(avatarTransform.getOrigin()) - offset);             
        m_avatarData->unlock();
    }
}

