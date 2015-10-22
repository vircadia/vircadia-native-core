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


#ifndef hifi_MyCharacterController_h
#define hifi_MyCharacterController_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <BulletUtil.h>
#include <CharacterController.h>
#include <SharedUtil.h>

class btCollisionShape;
class MyAvatar;

class MyCharacterController : public CharacterController {
public:
    MyCharacterController(MyAvatar* avatar);
    ~MyCharacterController ();

    // TODO: implement these when needed
    virtual void setVelocityForTimeInterval(const btVector3 &velocity, btScalar timeInterval) override { assert(false); }
    virtual void reset(btCollisionWorld* collisionWorld) override { }
    virtual void warp(const btVector3& origin) override { }
    virtual void debugDraw(btIDebugDraw* debugDrawer) override { }
    virtual void setUpInterpolate(bool value) override { }

    // overrides from btCharacterControllerInterface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime) override {
        preStep(collisionWorld);
        playerStep(collisionWorld, deltaTime);
    }
    virtual void preStep(btCollisionWorld* collisionWorld) override;
    virtual void playerStep(btCollisionWorld* collisionWorld, btScalar dt) override;
    virtual bool canJump() const override { assert(false); return false; } // never call this
    virtual void jump() override; // call this every frame the jump button is pressed
    virtual bool onGround() const override;

    // overrides from CharacterController
    virtual void preSimulation() override;
    virtual void postSimulation() override;

    bool isHovering() const { return _isHovering; }
    void setHovering(bool enabled);

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled && _dynamicsWorld; }

    void setLocalBoundingBox(const glm::vec3& corner, const glm::vec3& scale);

    virtual void updateShapeIfNecessary() override;

    void setAvatarPositionAndOrientation( const glm::vec3& position, const glm::quat& orientation);
    void getAvatarPositionAndOrientation(glm::vec3& position, glm::quat& rotation) const;

    void setTargetVelocity(const glm::vec3& velocity);
    void setHMDVelocity(const glm::vec3& velocity);
    glm::vec3 getHMDShift() const { return _lastStepDuration * bulletToGLM(_hmdVelocity); }

    glm::vec3 getLinearVelocity() const;

protected:
    void updateUpAxis(const glm::quat& rotation);

protected:
    btVector3 _currentUp;
    btVector3 _walkVelocity;
    btVector3 _hmdVelocity;
    btTransform _avatarBodyTransform;

    glm::vec3 _shapeLocalOffset;
    glm::vec3 _boxScale; // used to compute capsule shape

    quint64 _jumpToHoverStart;

    MyAvatar* _avatar { nullptr };

    btScalar _halfHeight;
    btScalar _radius;

    btScalar _floorDistance;

    btScalar _gravity;

    btScalar _jumpSpeed;
    btScalar _lastStepDuration;

    bool _enabled;
    bool _isOnGround;
    bool _isJumping;
    bool _isFalling;
    bool _isHovering;
    bool _isPushingUp;
};

#endif // hifi_MyCharacterController_h
