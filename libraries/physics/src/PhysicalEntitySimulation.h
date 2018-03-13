//
//  PhysicalEntitySimulation.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.04.27
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicalEntitySimulation_h
#define hifi_PhysicalEntitySimulation_h

#include <stdint.h>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <EntityItem.h>
#include <EntitySimulation.h>

#include "PhysicsEngine.h"
#include "EntityMotionState.h"

class PhysicalEntitySimulation;
using PhysicalEntitySimulationPointer = std::shared_ptr<PhysicalEntitySimulation>;
using SetOfEntityMotionStates = QSet<EntityMotionState*>;

class PhysicalEntitySimulation : public EntitySimulation {
    Q_OBJECT
public:
    PhysicalEntitySimulation();
    ~PhysicalEntitySimulation();

    void init(EntityTreePointer tree, PhysicsEnginePointer engine, EntityEditPacketSender* packetSender);

    virtual void addDynamic(EntityDynamicPointer dynamic) override;
    virtual void applyDynamicChanges() override;

    virtual void takeDeadEntities(SetOfEntities& deadEntities) override;

signals:
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

protected: // only called by EntitySimulation
    // overrides for EntitySimulation
    virtual void updateEntitiesInternal(const quint64& now) override;
    virtual void addEntityInternal(EntityItemPointer entity) override;
    virtual void removeEntityInternal(EntityItemPointer entity) override;
    virtual void changeEntityInternal(EntityItemPointer entity) override;
    virtual void clearEntitiesInternal() override;

public:
    virtual void prepareEntityForDelete(EntityItemPointer entity) override;

    const VectorOfMotionStates& getObjectsToRemoveFromPhysics();
    void deleteObjectsRemovedFromPhysics();

    void getObjectsToAddToPhysics(VectorOfMotionStates& result);
    void setObjectsToChange(const VectorOfMotionStates& objectsToChange);
    void getObjectsToChange(VectorOfMotionStates& result);

    void handleDeactivatedMotionStates(const VectorOfMotionStates& motionStates);
    void handleChangedMotionStates(const VectorOfMotionStates& motionStates);
    void handleCollisionEvents(const CollisionEvents& collisionEvents);

    EntityEditPacketSender* getPacketSender() { return _entityPacketSender; }

private:
    SetOfEntities _entitiesToAddToPhysics;
    SetOfEntities _entitiesToRemoveFromPhysics;

    VectorOfMotionStates _objectsToDelete;

    SetOfEntityMotionStates _pendingChanges; // EntityMotionStates already in PhysicsEngine that need their physics changed
    SetOfEntityMotionStates _outgoingChanges; // EntityMotionStates for which we may need to send updates to entity-server

    SetOfMotionStates _physicalObjects; // MotionStates of entities in PhysicsEngine

    PhysicsEnginePointer _physicsEngine = nullptr;
    EntityEditPacketSender* _entityPacketSender = nullptr;

    uint32_t _lastStepSendPackets { 0 };
};


typedef std::shared_ptr<PhysicalEntitySimulation> PhysicalEntitySimulationPointer;

#endif // hifi_PhysicalEntitySimulation_h
