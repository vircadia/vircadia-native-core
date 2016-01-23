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

    QMutexLocker lock(&_mutex);
    SetOfEntities::iterator itemItr = _entitiesWithSimulator.begin();
    while (itemItr != _entitiesWithSimulator.end()) {
        EntityItemPointer entity = *itemItr;
        if (entity->getSimulatorID().isNull()) {
            itemItr = _entitiesWithSimulator.erase(itemItr);
        } else if (now - entity->getLastChangedOnServer() >= AUTO_REMOVE_SIMULATION_OWNER_USEC) {
            SharedNodePointer ownerNode = nodeList->nodeWithUUID(entity->getSimulatorID());
            if (ownerNode.isNull() || !ownerNode->isAlive()) {
                qCDebug(entities) << "auto-removing simulation owner" << entity->getSimulatorID();
                entity->clearSimulationOwnership();
                itemItr = _entitiesWithSimulator.erase(itemItr);
                // zero the velocity on this entity so that it doesn't drift far away
                entity->setVelocity(glm::vec3(0.0f));
            } else {
                ++itemItr;
            }
        } else {
            ++itemItr;
        }
    }
}

void SimpleEntitySimulation::addEntityInternal(EntityItemPointer entity) {
    EntitySimulation::addEntityInternal(entity);
    if (!entity->getSimulatorID().isNull()) {
        QMutexLocker lock(&_mutex);
        _entitiesWithSimulator.insert(entity);
    }
}

void SimpleEntitySimulation::removeEntityInternal(EntityItemPointer entity) {
    EntitySimulation::removeEntityInternal(entity);
    QMutexLocker lock(&_mutex);
    _entitiesWithSimulator.remove(entity);
}

void SimpleEntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    EntitySimulation::changeEntityInternal(entity);
    if (!entity->getSimulatorID().isNull()) {
        QMutexLocker lock(&_mutex);
        _entitiesWithSimulator.insert(entity);
    }
    entity->clearDirtyFlags();
}

void SimpleEntitySimulation::clearEntitiesInternal() {
    _entitiesWithSimulator.clear();
}

