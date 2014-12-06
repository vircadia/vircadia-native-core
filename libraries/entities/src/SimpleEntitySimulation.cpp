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


void SimpleEntitySimulation::updateEntitiesInternal(const quint64& now) {
    QSet<EntityItem*>::iterator itemItr = _movingEntities.begin();
    while (itemItr != _movingEntities.end()) {
        EntityItem* entity = *itemItr;
        if (!entity->isMoving()) {
            itemItr = _movingEntities.erase(itemItr);
            _movableButStoppedEntities.insert(entity);
        } else {
            entity->simulate(now);
            _entitiesToBeSorted.insert(entity);
        }
    }
}

void SimpleEntitySimulation::addEntityInternal(EntityItem* entity) {
    if (entity->getCollisionsWillMove()) {
        if (entity->isMoving()) {
            _movingEntities.insert(entity);
        } else {
            _movableButStoppedEntities.insert(entity);
        }
    }
}

void SimpleEntitySimulation::removeEntityInternal(EntityItem* entity) {
    _movingEntities.remove(entity);
    _movableButStoppedEntities.remove(entity);
}

const int SIMPLE_SIMULATION_DIRTY_FLAGS = EntityItem::DIRTY_VELOCITY | EntityItem::DIRTY_MOTION_TYPE;

void SimpleEntitySimulation::entityChangedInternal(EntityItem* entity) {
    int dirtyFlags = entity->getDirtyFlags();
    if (dirtyFlags & SIMPLE_SIMULATION_DIRTY_FLAGS) {
        if (entity->getCollisionsWillMove()) {
            if (entity->isMoving()) {
                _movingEntities.insert(entity);
                _movableButStoppedEntities.remove(entity);
            } else {
                _movingEntities.remove(entity);
                _movableButStoppedEntities.insert(entity);
            }
        } else {
            _movingEntities.remove(entity);
            _movableButStoppedEntities.remove(entity);
        }
    }
}

void SimpleEntitySimulation::clearEntitiesInternal() {
    _movingEntities.clear();
    _movableButStoppedEntities.clear();
}

