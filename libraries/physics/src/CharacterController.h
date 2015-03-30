/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2008 Erwin Coumans  http://bulletphysics.com
2015.03.25 -- modified by Andrew Meadows andrew@highfidelity.io

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
   If you use this software in a product, an acknowledgment in the product documentation would be appreciated but 
   is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#ifndef hifi_CharacterController_h
#define hifi_CharacterController_h

#include <AvatarData.h>

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btCharacterControllerInterface.h>
#include <BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h>


class btConvexShape;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;

///CharacterController is a custom version of btKinematicCharacterController

///btKinematicCharacterController is an object that supports a sliding motion in a world.
///It uses a ghost object and convex sweep test to test for upcoming collisions. This is combined with discrete collision detection to recover from penetrations.
///Interaction between btKinematicCharacterController and dynamic rigid bodies needs to be explicity implemented by the user.


ATTRIBUTE_ALIGNED16(class) CharacterController : public btCharacterControllerInterface
{
protected:

    AvatarData* _avatarData = NULL;
    btPairCachingGhostObject* _ghostObject;

    btConvexShape* _convexShape;//is also in _ghostObject, but it needs to be convex, so we store it here to avoid upcast
    btScalar _radius;
    btScalar _halfHeight;

    btScalar _verticalVelocity;
    btScalar _verticalOffset; // fall distance from velocity this frame
    btScalar _maxFallSpeed;
    btScalar _jumpSpeed;
    btScalar _maxJumpHeight;
    btScalar _maxSlopeRadians; // Slope angle that is set (used for returning the exact value)
    btScalar _maxSlopeCosine;  // Cosine equivalent of _maxSlopeRadians (calculated once when set, for optimization)
    btScalar _gravity;

    btScalar _stepUpHeight; // height of stepUp prior to stepForward
    btScalar _stepDownHeight; // height of stepDown

    btScalar _addedMargin;//@todo: remove this and fix the code

    ///this is the desired walk direction, set by the user
    btVector3 _walkDirection;
    btVector3 _normalizedDirection;

    //some internal variables
    btVector3 _currentPosition;
    btQuaternion _currentRotation;
    btVector3 _targetPosition;
    glm::vec3 _lastPosition;
    btScalar  _lastStepUp;

    ///keep track of the contact manifolds
    btManifoldArray _manifoldArray;

    bool _touchingContact;
    btVector3 _floorNormal; // points from object to character

    bool _enabled;
    bool _isOnGround;
    bool _isJumping;
    bool _isHovering;
    quint64 _jumpToHoverStart;
    btScalar _velocityTimeInterval;
    btScalar _stepDt;
    uint32_t _pendingFlags;

    glm::vec3 _shapeLocalOffset;
    glm::vec3 _boxScale; // used to compute capsule shape

    btDynamicsWorld* _dynamicsWorld = NULL;

    btVector3 computeReflectionDirection(const btVector3& direction, const btVector3& normal);
    btVector3 parallelComponent(const btVector3& direction, const btVector3& normal);
    btVector3 perpindicularComponent(const btVector3& direction, const btVector3& normal);

    bool recoverFromPenetration(btCollisionWorld* collisionWorld);
    void scanDown(btCollisionWorld* collisionWorld);
    void stepUp(btCollisionWorld* collisionWorld);
    void updateTargetPositionBasedOnCollision(const btVector3& hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));
    void stepForward(btCollisionWorld* collisionWorld, const btVector3& walkMove);
    void stepDown(btCollisionWorld* collisionWorld, btScalar dt);
    void createShapeAndGhost();
public:

    BT_DECLARE_ALIGNED_ALLOCATOR();

    CharacterController(AvatarData* avatarData);
    ~CharacterController();


    ///btActionInterface interface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) {
        preStep(collisionWorld);
        playerStep(collisionWorld, deltaTime);
    }

    ///btActionInterface interface
    void debugDraw(btIDebugDraw* debugDrawer);

    /// This should probably be called setPositionIncrementPerSimulatorStep.
    /// This is neither a direction nor a velocity, but the amount to
    /// increment the position each simulation iteration, regardless
    /// of dt.
    /// This call will reset any velocity set by setVelocityForTimeInterval().
    virtual void setWalkDirection(const btVector3& walkDirection);

    /// Caller provides a velocity with which the character should move for
    /// the given time period.  After the time period, velocity is reset
    /// to zero.
    /// This call will reset any walk direction set by setWalkDirection().
    /// Negative time intervals will result in no motion.
    virtual void setVelocityForTimeInterval(const btVector3& velocity,
            btScalar timeInterval);

    virtual void reset(btCollisionWorld* collisionWorld );
    virtual void warp(const btVector3& origin);

    virtual void preStep(btCollisionWorld* collisionWorld);
    virtual void playerStep(btCollisionWorld* collisionWorld, btScalar dt);

    virtual bool canJump() const;
    virtual void jump();
    virtual bool onGround() const;

    void setMaxFallSpeed(btScalar speed);
    void setJumpSpeed(btScalar jumpSpeed);
    void setMaxJumpHeight(btScalar maxJumpHeight);

    void setGravity(btScalar gravity);
    btScalar getGravity() const;

    /// The max slope determines the maximum angle that the controller can walk up.
    /// The slope angle is measured in radians.
    void setMaxSlope(btScalar slopeRadians);
    btScalar getMaxSlope() const;

    btPairCachingGhostObject* getGhostObject();

    void setUpInterpolate(bool value);

    bool needsRemoval() const;
    bool needsAddition() const;
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }
    void setDynamicsWorld(btDynamicsWorld* world);

    void setLocalBoundingBox(const glm::vec3& corner, const glm::vec3& scale);
    bool needsShapeUpdate() const;
    void updateShapeIfNecessary();

    void preSimulation(btScalar timeStep);
    void postSimulation();
};

#endif // hifi_CharacterController_h
