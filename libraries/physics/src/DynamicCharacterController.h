#ifndef hifi_DynamicCharacterController_h
#define hifi_DynamicCharacterController_h

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btCharacterControllerInterface.h>

#include <AvatarData.h>

class btCollisionShape;
class btRigidBody;
class btCollisionWorld;

///DynamicCharacterController is obsolete/unsupported at the moment
class DynamicCharacterController : public btCharacterControllerInterface
{
protected:
    btScalar _halfHeight;
    btScalar _radius;
    btCollisionShape* _shape;
    btRigidBody* _rigidBody;

    btVector3 _currentUp;
    btVector3 _raySource[2];
    btVector3 _rayTarget[2];
    btScalar _rayLambda[2];
    btVector3 _rayNormal[2];

    btVector3 _walkVelocity;
    //btScalar _turnVelocity;

    glm::vec3 _shapeLocalOffset;
    glm::vec3 _boxScale; // used to compute capsule shape
    AvatarData* _avatarData = NULL;

    bool _enabled;
    bool _isOnGround;
    bool _isJumping;
    bool _isHovering;
//    quint64 _jumpToHoverStart;
//    btScalar _velocityTimeInterval;
//    btScalar _stepDt;
    uint32_t _pendingFlags;

    btDynamicsWorld* _dynamicsWorld = NULL;

    btScalar _jumpSpeed;

public:
    DynamicCharacterController(AvatarData* avatarData);
    ~DynamicCharacterController ();

//    void setup(btScalar height = 2.0, btScalar width = 0.25, btScalar stepHeight = 0.25);
//    void destroy ();

    virtual void setWalkDirection(const btVector3& walkDirection);
    virtual void setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval) { assert(false); }

    virtual void reset(btCollisionWorld* collisionWorld);
    virtual void warp(const btVector3& origin);
    virtual void registerPairCacheAndDispatcher(btOverlappingPairCache* pairCache, btCollisionDispatcher* dispatcher);

    btCollisionObject* getCollisionObject();

    ///btActionInterface interface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) {
        preStep(collisionWorld);
        playerStep(collisionWorld, deltaTime);
    }

    virtual void debugDraw(btIDebugDraw* debugDrawer);

    void setUpInterpolate(bool value);

    virtual void preStep(btCollisionWorld* collisionWorld);
    virtual void playerStep(btCollisionWorld* collisionWorld, btScalar dt);

    virtual bool canJump() const;
    virtual void jump();
    virtual bool onGround() const;

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

//virtual void    setWalkDirectio(const btVector3 &walkDirection)=0
//virtual void    setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval)=0
//virtual void    reset()=0
//virtual void    warp(const btVector3 &origin)=0
//virtual void    preStep(btCollisionWorld *collisionWorld)=0
//virtual void    playerStep(btCollisionWorld *collisionWorld, btScalar dt)=0
//virtual bool    canJump() const =0
//virtual void    jump()=0
//virtual bool    onGround() const =0

#endif // hifi_DynamicCharacterController_h
