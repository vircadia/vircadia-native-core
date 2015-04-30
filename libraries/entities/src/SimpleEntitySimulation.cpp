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
    // now is usecTimestampNow()
    QSet<EntityItem*>::iterator itemItr = _movingEntities.begin();
    while (itemItr != _movingEntities.end()) {
        EntityItem* entity = *itemItr;
        if (!entity->isMoving()) {
            itemItr = _movingEntities.erase(itemItr);
            _movableButStoppedEntities.insert(entity);
        } else {
            entity->simulate(now);
            _entitiesToBeSorted.insert(entity);
            ++itemItr;
        }
    }

    // If an Entity has a simulation owner and we don't get an update for some amount of time,
    // clear the owner.  This guards against an interface failing to release the Entity when it
    // has finished simulating it.
    itemItr = _hasSimulationOwnerEntities.begin();
    while (itemItr != _hasSimulationOwnerEntities.end()) {
        EntityItem* entity = *itemItr;
        if (entity->getSimulatorID().isNull()) {
            itemItr = _hasSimulationOwnerEntities.erase(itemItr);
        } else if (usecTimestampNow() - entity->getLastChangedOnServer() >= AUTO_REMOVE_SIMULATION_OWNER_USEC) {
            qCDebug(entities) << "auto-removing simulation owner" << entity->getSimulatorID();
            entity->setSimulatorID(QUuid());
            itemItr = _hasSimulationOwnerEntities.erase(itemItr);
        } else {
            ++itemItr;
        }
    }
}

void SimpleEntitySimulation::addEntityInternal(EntityItem* entity) {
    if (entity->isMoving()) {
        _movingEntities.insert(entity);
    } else if (entity->getCollisionsWillMove()) {
        _movableButStoppedEntities.insert(entity);
    }
    if (!entity->getSimulatorID().isNull()) {
        _hasSimulationOwnerEntities.insert(entity);
    }
}

void SimpleEntitySimulation::removeEntityInternal(EntityItem* entity) {
    _movingEntities.remove(entity);
    _movableButStoppedEntities.remove(entity);
    _hasSimulationOwnerEntities.remove(entity);
}

const int SIMPLE_SIMULATION_DIRTY_FLAGS = EntityItem::DIRTY_VELOCITY | EntityItem::DIRTY_MOTION_TYPE;

void SimpleEntitySimulation::entityChangedInternal(EntityItem* entity) {
    int dirtyFlags = entity->getDirtyFlags();
    if (dirtyFlags & SIMPLE_SIMULATION_DIRTY_FLAGS) {
        if (entity->isMoving()) {
            _movingEntities.insert(entity);
        } else if (entity->getCollisionsWillMove()) {
            _movableButStoppedEntities.remove(entity);
        } else {
            _movingEntities.remove(entity);
            _movableButStoppedEntities.remove(entity);
        }
        if (!entity->getSimulatorID().isNull()) {
            _hasSimulationOwnerEntities.insert(entity);
        }
    }
    entity->clearDirtyFlags();
}

void SimpleEntitySimulation::clearEntitiesInternal() {
    _movingEntities.clear();
    _movableButStoppedEntities.clear();
    _hasSimulationOwnerEntities.clear();
}

