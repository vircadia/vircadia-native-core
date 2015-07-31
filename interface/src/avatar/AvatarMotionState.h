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
    AvatarMotionState(Avatar* avatar, btCollisionShape* shape);
    ~AvatarMotionState();

    virtual MotionType getMotionType() const { return _motionType; }

    virtual uint32_t getAndClearIncomingDirtyFlags();

    virtual MotionType computeObjectMotionType() const;

    virtual bool isMoving() const;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans);


    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    virtual float getObjectRestitution() const;
    virtual float getObjectFriction() const;
    virtual float getObjectLinearDamping() const;
    virtual float getObjectAngularDamping() const;
    
    virtual glm::vec3 getObjectPosition() const;
    virtual glm::quat getObjectRotation() const;
    virtual glm::vec3 getObjectLinearVelocity() const;
    virtual glm::vec3 getObjectAngularVelocity() const;
    virtual glm::vec3 getObjectGravity() const;

    virtual const QUuid& getObjectID() const;

    virtual QUuid getSimulatorID() const;

    void setBoundingBox(const glm::vec3& corner, const glm::vec3& diagonal);

    void addDirtyFlags(uint32_t flags) { _dirtyFlags |= flags; }

    virtual int16_t computeCollisionGroup();

    friend class AvatarManager;

protected:
    virtual btCollisionShape* computeNewShape();
    virtual void clearObjectBackPointer();
    Avatar* _avatar;
    uint32_t _dirtyFlags;
};

typedef QSet<AvatarMotionState*> SetOfAvatarMotionStates;

#endif // hifi_AvatarMotionState_h
