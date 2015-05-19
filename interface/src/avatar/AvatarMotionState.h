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

#include <QVector>

#include <ObjectMotionState.h>

class Avatar;

class AvatarMotionState : public ObjectMotionState {
public:
    AvatarMotionState(Avatar* avatar, btCollisionShape* shape);
    ~AvatarMotionState();

    virtual void handleEasyChanges(uint32_t flags);
    virtual void handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine);

    virtual void updateBodyMaterialProperties();
    virtual void updateBodyVelocities();

    virtual MotionType getMotionType() const { return _motionType; }

    virtual uint32_t getAndClearIncomingDirtyFlags() const = 0;

    virtual MotionType computeObjectMotionType() const = 0;
    virtual void computeObjectShapeInfo(ShapeInfo& shapeInfo) = 0;

    virtual bool isMoving() const = 0;

    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    virtual float getObjectRestitution() const = 0;
    virtual float getObjectFriction() const = 0;
    virtual float getObjectLinearDamping() const = 0;
    virtual float getObjectAngularDamping() const = 0;
    
    virtual glm::vec3 getObjectPosition() const = 0;
    virtual glm::quat getObjectRotation() const = 0;
    virtual const glm::vec3& getObjectLinearVelocity() const = 0;
    virtual const glm::vec3& getObjectAngularVelocity() const = 0;
    virtual const glm::vec3& getObjectGravity() const = 0;

    virtual const QUuid& getObjectID() const = 0;

    virtual QUuid getSimulatorID() const = 0;
    virtual void bump() = 0;

protected:
    virtual void setMotionType(MotionType motionType);
    Avatar* _avatar;
};

typedef QVector<AvatarMotionState*> VectorOfAvatarMotionStates;

#endif // hifi_AvatarMotionState_h
