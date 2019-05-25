//
//  AvatarMotionState.h
//  interface/src/avatar/
//
//  Created by Andrew Meadows 2015.05.14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMotionState_h
#define hifi_AvatarMotionState_h

#include <QSet>

#include <ObjectMotionState.h>
#include <BulletUtil.h>

#include "OtherAvatar.h"

class AvatarMotionState : public ObjectMotionState {
public:
    AvatarMotionState(OtherAvatarPointer avatar, const btCollisionShape* shape);

    void handleEasyChanges(uint32_t& flags) override;

    PhysicsMotionType getMotionType() const override { return _motionType; }

    uint32_t getIncomingDirtyFlags() const override;
    void clearIncomingDirtyFlags(uint32_t mask = DIRTY_PHYSICS_FLAGS) override;

    PhysicsMotionType computePhysicsMotionType() const override;

    bool isMoving() const override;

    // this relays incoming position/rotation to the RigidBody
    void getWorldTransform(btTransform& worldTrans) const override;

    // this relays outgoing position/rotation to the EntityItem
    void setWorldTransform(const btTransform& worldTrans) override;


    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    // pure virtual overrides from ObjectMotionState
    float getObjectRestitution() const override;
    float getObjectFriction() const override;
    float getObjectLinearDamping() const override;
    float getObjectAngularDamping() const override;

    glm::vec3 getObjectPosition() const override;
    glm::quat getObjectRotation() const override;
    glm::vec3 getObjectLinearVelocity() const override;
    glm::vec3 getObjectAngularVelocity() const override;
    glm::vec3 getObjectGravity() const override;

    const QUuid getObjectID() const override;

    QString getName() const override;
    ShapeType getShapeType() const override { return SHAPE_TYPE_CAPSULE_Y; }
    QUuid getSimulatorID() const override;

    void setBoundingBox(const glm::vec3& corner, const glm::vec3& diagonal);

    void addDirtyFlags(uint32_t flags) { _dirtyFlags |= flags; }

    void setCollisionGroup(int32_t group) { _collisionGroup = group; }
    int32_t getCollisionGroup() { return _collisionGroup; }

    void computeCollisionGroupAndMask(int32_t& group, int32_t& mask) const override;

    float getMass() const override;

    friend class AvatarManager;
    friend class Avatar;

protected:
    void setRigidBody(btRigidBody* body) override;
    void setShape(const btCollisionShape* shape) override;
    void cacheShapeDiameter();

    // the dtor had been made protected to force the compiler to verify that it is only
    // ever called by the Avatar class dtor.
    ~AvatarMotionState();

    OtherAvatarPointer _avatar;
    float _diameter { 0.0f };
    int32_t _collisionGroup;
    uint32_t _dirtyFlags;
};

#endif // hifi_AvatarMotionState_h
