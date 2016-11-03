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

class Avatar;

class AvatarMotionState : public ObjectMotionState {
public:
    AvatarMotionState(Avatar* avatar, const btCollisionShape* shape);

    virtual PhysicsMotionType getMotionType() const override { return _motionType; }

    virtual uint32_t getIncomingDirtyFlags() override;
    virtual void clearIncomingDirtyFlags() override;

    virtual PhysicsMotionType computePhysicsMotionType() const override;

    virtual bool isMoving() const override;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const override;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans) override;


    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    // pure virtual overrides from ObjectMotionState
    virtual float getObjectRestitution() const override;
    virtual float getObjectFriction() const override;
    virtual float getObjectLinearDamping() const override;
    virtual float getObjectAngularDamping() const override;

    virtual glm::vec3 getObjectPosition() const override;
    virtual glm::quat getObjectRotation() const override;
    virtual glm::vec3 getObjectLinearVelocity() const override;
    virtual glm::vec3 getObjectAngularVelocity() const override;
    virtual glm::vec3 getObjectGravity() const override;

    virtual const QUuid getObjectID() const override;

    virtual QUuid getSimulatorID() const override;

    void setBoundingBox(const glm::vec3& corner, const glm::vec3& diagonal);

    void addDirtyFlags(uint32_t flags) { _dirtyFlags |= flags; }

    virtual void computeCollisionGroupAndMask(int16_t& group, int16_t& mask) const override;

    friend class AvatarManager;
    friend class Avatar;

protected:
    // the dtor had been made protected to force the compiler to verify that it is only
    // ever called by the Avatar class dtor.
    ~AvatarMotionState();

    virtual bool isReadyToComputeShape() const override { return true; }
    virtual const btCollisionShape* computeNewShape() override;

    // The AvatarMotionState keeps a RAW backpointer to its Avatar because all AvatarMotionState
    // instances are "owned" by their corresponding Avatar instance and are deleted in the Avatar dtor.
    // In other words, it is impossible for the Avatar to be deleted out from under its MotionState.
    // In conclusion: weak pointer shennanigans would be pure overhead.
    Avatar* _avatar; // do NOT use smartpointer here, no need for weakpointer

    uint32_t _dirtyFlags;
};

typedef QSet<AvatarMotionState*> SetOfAvatarMotionStates;

#endif // hifi_AvatarMotionState_h
