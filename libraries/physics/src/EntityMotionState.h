//
//  EntityMotionState.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.06
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityMotionState_h
#define hifi_EntityMotionState_h

#include <EntityTypes.h>
#include <AACube.h>

#include "ObjectMotionState.h"


// From the MotionState's perspective:
//      Inside = physics simulation
//      Outside = external agents (scripts, user interaction, other simulations)

class EntityMotionState : public ObjectMotionState {
public:

    EntityMotionState(btCollisionShape* shape, EntityItemPointer item);
    virtual ~EntityMotionState();

    void updateServerPhysicsVariables();
    virtual bool handleEasyChanges(uint32_t& flags);
    virtual bool handleHardAndEasyChanges(uint32_t& flags, PhysicsEngine* engine);

    /// \return PhysicsMotionType based on params set in EntityItem
    virtual PhysicsMotionType computePhysicsMotionType() const;

    virtual bool isMoving() const;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const override;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans) override;

    bool isCandidateForOwnership(const QUuid& sessionID) const;
    bool remoteSimulationOutOfSync(uint32_t simulationStep);
    bool shouldSendUpdate(uint32_t simulationStep, const QUuid& sessionID);
    void sendUpdate(OctreeEditPacketSender* packetSender, const QUuid& sessionID, uint32_t step);

    virtual uint32_t getIncomingDirtyFlags();
    virtual void clearIncomingDirtyFlags();

    void incrementAccelerationNearlyGravityCount() { _accelerationNearlyGravityCount++; }
    void resetAccelerationNearlyGravityCount() { _accelerationNearlyGravityCount = 0; }
    quint8 getAccelerationNearlyGravityCount() { return _accelerationNearlyGravityCount; }

    virtual float getObjectRestitution() const override { return _entity->getRestitution(); }
    virtual float getObjectFriction() const override { return _entity->getFriction(); }
    virtual float getObjectLinearDamping() const override { return _entity->getDamping(); }
    virtual float getObjectAngularDamping() const override { return _entity->getAngularDamping(); }

    virtual glm::vec3 getObjectPosition() const override { return _entity->getPosition() - ObjectMotionState::getWorldOffset(); }
    virtual glm::quat getObjectRotation() const override { return _entity->getRotation(); }
    virtual glm::vec3 getObjectLinearVelocity() const override { return _entity->getVelocity(); }
    virtual glm::vec3 getObjectAngularVelocity() const override { return _entity->getAngularVelocity(); }
    virtual glm::vec3 getObjectGravity() const override { return _entity->getGravity(); }
    virtual glm::vec3 getObjectLinearVelocityChange() const override;

    virtual const QUuid& getObjectID() const override { return _entity->getID(); }

    virtual quint8 getSimulationPriority() const override;
    virtual QUuid getSimulatorID() const override;
    virtual void bump(quint8 priority) override;

    EntityItemPointer getEntity() const { return _entityPtr.lock(); }

    void resetMeasuredBodyAcceleration();
    void measureBodyAcceleration();

    virtual QString getName() const override;

    virtual void computeCollisionGroupAndMask(int16_t& group, int16_t& mask) const;

    // eternal logic can suggest a simuator priority bid for the next outgoing update
    void setOutgoingPriority(quint8 priority);

    friend class PhysicalEntitySimulation;

protected:
    #ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
    bool entityTreeIsLocked() const;
    #endif

    virtual bool isReadyToComputeShape() const override;
    virtual btCollisionShape* computeNewShape();
    virtual void setMotionType(PhysicsMotionType motionType);

    // In the glorious future (when entities lib depends on physics lib) the EntityMotionState will be
    // properly "owned" by the EntityItem and will be deleted by it in the dtor.  In pursuit of that
    // state of affairs we can't keep a real EntityItemPointer as data member (it would produce a
    // recursive dependency).  Instead we keep a EntityItemWeakPointer to break that dependency while
    // still granting us the capability to generate EntityItemPointers as necessary (for external data
    // structures that use the MotionState to get to the EntityItem).
    EntityItemWeakPointer _entityPtr;
    // Meanwhile we also keep a raw EntityItem* for internal stuff where the pointer is guaranteed valid.
    EntityItem* _entity;

    bool _sentInactive;   // true if body was inactive when we sent last update

    // these are for the prediction of the remote server's simple extrapolation
    uint32_t _lastStep; // last step of server extrapolation
    glm::vec3 _serverPosition;    // in simulation-frame (not world-frame)
    glm::quat _serverRotation;
    glm::vec3 _serverVelocity;
    glm::vec3 _serverAngularVelocity; // radians per second
    glm::vec3 _serverGravity;
    glm::vec3 _serverAcceleration;
    QByteArray _serverActionData;

    uint32_t _lastMeasureStep;
    glm::vec3 _lastVelocity;
    glm::vec3 _measuredAcceleration;
    float _measuredDeltaTime;

    quint8 _accelerationNearlyGravityCount;
    quint64 _nextOwnershipBid = NO_PRORITY;
    uint32_t _loopsWithoutOwner;
    quint8 _outgoingPriority = NO_PRORITY;
};

#endif // hifi_EntityMotionState_h
