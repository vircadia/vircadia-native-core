//
//  DetailedMotionState.h
//  interface/src/avatar/
//
//  Created by Luis Cuenca 1/11/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DetailedMotionState_h
#define hifi_DetailedMotionState_h

#include <QSet>

#include <ObjectMotionState.h>
#include <BulletUtil.h>

#include "OtherAvatar.h"

class DetailedMotionState : public ObjectMotionState {
public:
    DetailedMotionState(AvatarPointer avatar, const btCollisionShape* shape, int jointIndex);

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
    ShapeType getShapeType() const override { return SHAPE_TYPE_HULL; }
    QUuid getSimulatorID() const override;

    void addDirtyFlags(uint32_t flags) { _dirtyFlags |= flags; }

    void computeCollisionGroupAndMask(int32_t& group, int32_t& mask) const override;

    float getMass() const override;
    void forceActive();
    QUuid getAvatarID() const { return _avatar->getID(); }
    int32_t getJointIndex() const { return _jointIndex; }
    void setIsBound(bool isBound, const std::vector<int32_t>& boundJoints) { _isBound = isBound; _boundJoints = boundJoints; }
    bool getIsBound(std::vector<int32_t>& boundJoints) const { boundJoints = _boundJoints; return _isBound; }

    friend class AvatarManager;
    friend class Avatar;

protected:
    void setRigidBody(btRigidBody* body) override;
    void setShape(const btCollisionShape* shape) override;

    // the dtor had been made protected to force the compiler to verify that it is only
    // ever called by the Avatar class dtor.
    ~DetailedMotionState();

    AvatarPointer _avatar;
    float _diameter { 0.0f };

    uint32_t _dirtyFlags;
    int32_t _jointIndex { -1 };
    OtherAvatarPointer _otherAvatar { nullptr };
    bool _isBound { false };
    std::vector<int32_t> _boundJoints;
};

#endif // hifi_DetailedMotionState_h
