#ifndef hifi_DynamicCharacterController_h
#define hifi_DynamicCharacterController_h

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btCharacterControllerInterface.h>

#include <AvatarData.h>

class btCollisionShape;
class btRigidBody;
class btCollisionWorld;

const int NUM_CHARACTER_CONTROLLER_RAYS = 2;

///DynamicCharacterController is obsolete/unsupported at the moment
class DynamicCharacterController : public btCharacterControllerInterface
{
protected:
    btScalar _halfHeight;
    btScalar _radius;
    btCollisionShape* _shape;
    btRigidBody* _rigidBody;

    btVector3 _currentUp;

    btScalar _floorDistance;

    btVector3 _walkVelocity;
    btScalar _gravity;

    glm::vec3 _shapeLocalOffset;
    glm::vec3 _boxScale; // used to compute capsule shape
    AvatarData* _avatarData = NULL;

    bool _enabled;
    bool _isOnGround;
    bool _isJumping;
    bool _isFalling;
    bool _isHovering;
    quint64 _jumpToHoverStart;
    uint32_t _pendingFlags;

    btDynamicsWorld* _dynamicsWorld = NULL;

    btScalar _jumpSpeed;

public:
    DynamicCharacterController(AvatarData* avatarData);
    ~DynamicCharacterController ();

    virtual void setWalkDirection(const btVector3& walkDirection);
    virtual void setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval) { assert(false); }

    // TODO: implement these when needed
    virtual void reset(btCollisionWorld* collisionWorld) { }
    virtual void warp(const btVector3& origin) { }
    virtual void debugDraw(btIDebugDraw* debugDrawer) { }
    virtual void setUpInterpolate(bool value) { }

    btCollisionObject* getCollisionObject() { return _rigidBody; }

    ///btActionInterface interface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) {
        preStep(collisionWorld);
        playerStep(collisionWorld, deltaTime);
    }

    virtual void preStep(btCollisionWorld* collisionWorld);
    virtual void playerStep(btCollisionWorld* collisionWorld, btScalar dt);

    virtual bool canJump() const;
    virtual void jump();
    virtual bool onGround() const;
    bool isHovering() const { return _isHovering; }
    void setHovering(bool enabled);

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

protected:
    void updateUpAxis(const glm::quat& rotation);
};

#endif // hifi_DynamicCharacterController_h
