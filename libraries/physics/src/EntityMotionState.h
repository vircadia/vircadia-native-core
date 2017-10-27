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
    void handleDeactivation();
    virtual void handleEasyChanges(uint32_t& flags) override;
    virtual bool handleHardAndEasyChanges(uint32_t& flags, PhysicsEngine* engine) override;

    /// \return PhysicsMotionType based on params set in EntityItem
    virtual PhysicsMotionType computePhysicsMotionType() const override;

    virtual bool isMoving() const override;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const override;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans) override;

    bool isCandidateForOwnership() const;
    bool remoteSimulationOutOfSync(uint32_t simulationStep);
    bool shouldSendUpdate(uint32_t simulationStep);
    void sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step);

    virtual uint32_t getIncomingDirtyFlags() override;
    virtual void clearIncomingDirtyFlags() override;

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

    virtual const QUuid getObjectID() const override { return _entity->getID(); }

    virtual uint8_t getSimulationPriority() const override;
    virtual QUuid getSimulatorID() const override;
    virtual void bump(uint8_t priority) override;

    EntityItemPointer getEntity() const { return _entityPtr.lock(); }

    void resetMeasuredBodyAcceleration();
    void measureBodyAcceleration();

    virtual QString getName() const override;

    virtual void computeCollisionGroupAndMask(int16_t& group, int16_t& mask) const override;

    bool isLocallyOwned() const override;
    bool shouldBeLocallyOwned() const override;

    friend class PhysicalEntitySimulation;

protected:
    // changes _outgoingPriority only if priority is larger
    void upgradeOutgoingPriority(uint8_t priority);

    #ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
    bool entityTreeIsLocked() const;
    #endif

    bool isReadyToComputeShape() const override;
    const btCollisionShape* computeNewShape() override;
    void setShape(const btCollisionShape* shape) override;
    void setMotionType(PhysicsMotionType motionType) override;

    // In the glorious future (when entities lib depends on physics lib) the EntityMotionState will be
    // properly "owned" by the EntityItem and will be deleted by it in the dtor.  In pursuit of that
    // state of affairs we can't keep a real EntityItemPointer as data member (it would produce a
    // recursive dependency).  Instead we keep a EntityItemWeakPointer to break that dependency while
    // still granting us the capability to generate EntityItemPointers as necessary (for external data
    // structures that use the MotionState to get to the EntityItem).
    EntityItemWeakPointer _entityPtr;
    // Meanwhile we also keep a raw EntityItem* for internal stuff where the pointer is guaranteed valid.
    EntityItem* _entity;

    bool _serverVariablesSet { false };
    glm::vec3 _serverPosition;    // in simulation-frame (not world-frame)
    glm::quat _serverRotation;
    glm::vec3 _serverVelocity;
    glm::vec3 _serverAngularVelocity; // radians per second
    glm::vec3 _serverGravity;
    glm::vec3 _serverAcceleration;
    QByteArray _serverActionData;

    glm::vec3 _lastVelocity;
    glm::vec3 _measuredAcceleration;
    quint64 _nextOwnershipBid { 0 };

    float _measuredDeltaTime;
    uint32_t _lastMeasureStep;
    uint32_t _lastStep; // last step of server extrapolation

    uint8_t _loopsWithoutOwner;
    mutable uint8_t _accelerationNearlyGravityCount;
    uint8_t _numInactiveUpdates { 1 };
    uint8_t _outgoingPriority { 0 };
};

#endif // hifi_EntityMotionState_h
