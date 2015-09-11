//
//  PhysicalEntitySimulation.h
//  libraries/physcis/src
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

typedef QSet<EntityMotionState*> SetOfEntityMotionStates;

class PhysicalEntitySimulation :public EntitySimulation {
public:
    PhysicalEntitySimulation();
    ~PhysicalEntitySimulation();

    void init(EntityTreePointer tree, PhysicsEnginePointer engine, EntityEditPacketSender* packetSender);

    virtual void addAction(EntityActionPointer action);
    virtual void applyActionChanges();

protected: // only called by EntitySimulation
    // overrides for EntitySimulation
    virtual void updateEntitiesInternal(const quint64& now);
    virtual void addEntityInternal(EntityItemPointer entity);
    virtual void removeEntityInternal(EntityItemPointer entity);
    virtual void changeEntityInternal(EntityItemPointer entity);
    virtual void clearEntitiesInternal();

public:
    VectorOfMotionStates& getObjectsToDelete();
    VectorOfMotionStates& getObjectsToAdd();
    void setObjectsToChange(VectorOfMotionStates& objectsToChange);
    VectorOfMotionStates& getObjectsToChange();

    void handleOutgoingChanges(VectorOfMotionStates& motionStates, const QUuid& sessionID);
    void handleCollisionEvents(CollisionEvents& collisionEvents);

    EntityEditPacketSender* getPacketSender() { return _entityPacketSender; }

private:
    // incoming changes
    SetOfEntityMotionStates _pendingRemoves; // EntityMotionStates to be removed from PhysicsEngine (and deleted)
    SetOfEntities _pendingAdds; // entities to be be added to PhysicsEngine (and a their EntityMotionState created)
    SetOfEntityMotionStates _pendingChanges; // EntityMotionStates already in PhysicsEngine that need their physics changed

    // outgoing changes
    SetOfEntityMotionStates _outgoingChanges; // EntityMotionStates for which we need to send updates to entity-server

    SetOfMotionStates _physicalObjects; // MotionStates of entities in PhysicsEngine
    VectorOfMotionStates _tempVector; // temporary array reference, valid immediately after getObjectsToRemove() (and friends)

    PhysicsEnginePointer _physicsEngine = nullptr;
    EntityEditPacketSender* _entityPacketSender = nullptr;

    uint32_t _lastStepSendPackets = 0;
};


typedef std::shared_ptr<PhysicalEntitySimulation> PhysicalEntitySimulationPointer;

#endif // hifi_PhysicalEntitySimulation_h
