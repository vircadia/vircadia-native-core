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

#include "SimpleEntitySimulation.h"

#include <DirtyOctreeElementOperator.h>

#include "EntityItem.h"
#include "EntitiesLogging.h"

const quint64 MIN_SIMULATION_OWNERSHIP_UPDATE_PERIOD = 2 * USECS_PER_SECOND;

void SimpleEntitySimulation::updateEntitiesInternal(const quint64& now) {
    if (_entitiesWithSimulator.size() == 0) {
        return;
    }

    if (now < _nextSimulationExpiry) {
        // nothing has expired yet
        return;
    }

    // If an Entity has a simulation owner but there has been no update for a while: clear the owner.
    // If an Entity goes ownerless for too long: zero velocity and remove from _entitiesWithSimulator.
    _nextSimulationExpiry = now + MIN_SIMULATION_OWNERSHIP_UPDATE_PERIOD;

    QMutexLocker lock(&_mutex);
    SetOfEntities::iterator itemItr = _entitiesWithSimulator.begin();
    while (itemItr != _entitiesWithSimulator.end()) {
        EntityItemPointer entity = *itemItr;
        quint64 expiry = entity->getLastChangedOnServer() + MIN_SIMULATION_OWNERSHIP_UPDATE_PERIOD;
        if (expiry < now) {
            if (entity->getSimulatorID().isNull()) {
                // no simulators are volunteering
                // zero the velocity on this entity so that it doesn't drift far away
                entity->setVelocity(Vectors::ZERO);
                entity->setAngularVelocity(Vectors::ZERO);
                entity->setAcceleration(Vectors::ZERO);
                // remove from list
                itemItr = _entitiesWithSimulator.erase(itemItr);
                continue;
            } else {
                // the simulator has stopped updating this object
                // clear ownership and restart timer, giving nearby simulators time to volunteer
                qCDebug(entities) << "auto-removing simulation owner " << entity->getSimulatorID();
                entity->clearSimulationOwnership();
            }
            entity->markAsChangedOnServer();
            // dirty all the tree elements that contain the entity
            DirtyOctreeElementOperator op(entity->getElement());
            getEntityTree()->recurseTreeWithOperator(&op);
        } else if (expiry < _nextSimulationExpiry) {
            _nextSimulationExpiry = expiry;
        }
        ++itemItr;
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

