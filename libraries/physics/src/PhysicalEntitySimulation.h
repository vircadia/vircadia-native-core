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

#include "PhysicsTypedefs.h"

class EntityMotionState;
class PhysicsEngine;
class ShapeManager;

class PhysicalEntitySimulation :public EntitySimulation {
public:
    PhysicalEntitySimulation();
    ~PhysicalEntitySimulation();

    void init(EntityTree* tree, PhysicsEngine* engine, ShapeManager* shapeManager, EntityEditPacketSender* packetSender);

protected: // only called by EntitySimulation
    // overrides for EntitySimulation
    void updateEntitiesInternal(const quint64& now);
    void addEntityInternal(EntityItem* entity);
    void removeEntityInternal(EntityItem* entity);
    void changeEntityInternal(EntityItem* entity);
    void clearEntitiesInternal();

public:
    VectorOfMotionStates& getObjectsToDelete();
    VectorOfMotionStates& getObjectsToAdd();
    VectorOfMotionStates& getObjectsToChange();

    void handleOutgoingChanges(VectorOfMotionStates& motionStates);

private:
    void bump(EntityItem* bumpEntity);

    // incoming changes
    SetOfEntities _pendingRemoves; // entities to be removed from PhysicsEngine (and their MotionState deleted)
    SetOfEntities _pendingAdds; // entities to be be added to PhysicsEngine (and a their MotionState created)
    SetOfEntities _pendingChanges; // entities already in PhysicsEngine that need their physics changed

    // outgoing changes
    QSet<EntityMotionState*> _outgoingChanges; // entities for which we need to send updates to entity-server

    SetOfMotionStates _physicalObjects; // MotionStates of entities in PhysicsEngine
    VectorOfMotionStates _tempVector; // temporary array, valid by reference immediately after call to getObjectsToRemove/Add/Update()

    ShapeManager* _shapeManager = nullptr;
    PhysicsEngine* _physicsEngine = nullptr;
    EntityEditPacketSender* _entityPacketSender = nullptr;

    uint32_t _lastStepSendPackets = 0;
};

#endif // hifi_PhysicalEntitySimulation_h
