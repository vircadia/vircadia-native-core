//
//  SimpleEntitySimulation.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//#include <PerfStat.h>

#include "EntityItem.h"
#include "SimpleEntitySimulation.h"
#include "EntitiesLogging.h"

const quint64 AUTO_REMOVE_SIMULATION_OWNER_USEC = 2 * USECS_PER_SECOND;

void SimpleEntitySimulation::updateEntitiesInternal(const quint64& now) {
    // If an Entity has a simulation owner and we don't get an update for some amount of time,
    // clear the owner.  This guards against an interface failing to release the Entity when it
    // has finished simulating it.
    auto nodeList = DependencyManager::get<LimitedNodeList>();

    SetOfEntities::iterator itemItr = _hasSimulationOwnerEntities.begin();
    while (itemItr != _hasSimulationOwnerEntities.end()) {
        EntityItem* entity = *itemItr;
        if (entity->getSimulatorID().isNull()) {
            itemItr = _hasSimulationOwnerEntities.erase(itemItr);
        } else if (now - entity->getLastChangedOnServer() >= AUTO_REMOVE_SIMULATION_OWNER_USEC) {
            SharedNodePointer ownerNode = nodeList->nodeWithUUID(entity->getSimulatorID());
            if (ownerNode.isNull() || !ownerNode->isAlive()) {
                qCDebug(entities) << "auto-removing simulation owner" << entity->getSimulatorID();
                entity->setSimulatorID(QUuid());
                itemItr = _hasSimulationOwnerEntities.erase(itemItr);
            }
        } else {
            ++itemItr;
        }
    }
}

void SimpleEntitySimulation::addEntityInternal(EntityItem* entity) {
    EntitySimulation::addEntityInternal(entity);
    if (!entity->getSimulatorID().isNull()) {
        _hasSimulationOwnerEntities.insert(entity);
    }
}

void SimpleEntitySimulation::removeEntityInternal(EntityItem* entity) {
    _hasSimulationOwnerEntities.remove(entity);
}

const int SIMPLE_SIMULATION_DIRTY_FLAGS = EntityItem::DIRTY_VELOCITIES | EntityItem::DIRTY_MOTION_TYPE;

void SimpleEntitySimulation::changeEntityInternal(EntityItem* entity) {
    if (!entity->getSimulatorID().isNull()) {
        _hasSimulationOwnerEntities.insert(entity);
    }
    entity->clearDirtyFlags();
}

void SimpleEntitySimulation::clearEntitiesInternal() {
    _hasSimulationOwnerEntities.clear();
}

