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

const quint64 MAX_OWNERLESS_PERIOD = 2 * USECS_PER_SECOND;

void SimpleEntitySimulation::clearOwnership(const QUuid& ownerID) {
    QMutexLocker lock(&_mutex);
    SetOfEntities::iterator itemItr = _entitiesWithSimulationOwner.begin();
    while (itemItr != _entitiesWithSimulationOwner.end()) {
        EntityItemPointer entity = *itemItr;
        if (entity->getSimulatorID() == ownerID) {
            // the simulator has abandonded this object --> remove from owned list
            qCDebug(entities) << "auto-removing simulation owner " << entity->getSimulatorID();
            itemItr = _entitiesWithSimulationOwner.erase(itemItr);

            if (entity->getDynamic() && entity->hasLocalVelocity()) {
                // it is still moving dynamically --> add to orphaned list
                _entitiesThatNeedSimulationOwner.insert(entity);
                quint64 expiry = entity->getLastChangedOnServer() + MAX_OWNERLESS_PERIOD;
                if (expiry < _nextOwnerlessExpiry) {
                    _nextOwnerlessExpiry = expiry;
                }
            }

            // remove ownership and dirty all the tree elements that contain the it
            entity->clearSimulationOwnership();
            entity->markAsChangedOnServer();
            DirtyOctreeElementOperator op(entity->getElement());
            getEntityTree()->recurseTreeWithOperator(&op);
        } else {
            ++itemItr;
        }
    }
}

void SimpleEntitySimulation::updateEntitiesInternal(const quint64& now) {
    if (now > _nextOwnerlessExpiry) {
        // search for ownerless objects that have expired
        QMutexLocker lock(&_mutex);
        _nextOwnerlessExpiry = -1;
        SetOfEntities::iterator itemItr = _entitiesThatNeedSimulationOwner.begin();
        while (itemItr != _entitiesThatNeedSimulationOwner.end()) {
            EntityItemPointer entity = *itemItr;
            quint64 expiry = entity->getLastChangedOnServer() + MAX_OWNERLESS_PERIOD;
            if (expiry < now) {
                // no simulators have volunteered ownership --> remove from list
                itemItr = _entitiesThatNeedSimulationOwner.erase(itemItr);

                if (entity->getSimulatorID().isNull() && entity->getDynamic() && entity->hasLocalVelocity()) {
                    // zero the derivatives
                    entity->setVelocity(Vectors::ZERO);
                    entity->setAngularVelocity(Vectors::ZERO);
                    entity->setAcceleration(Vectors::ZERO);

                    // dirty all the tree elements that contain it
                    entity->markAsChangedOnServer();
                    DirtyOctreeElementOperator op(entity->getElement());
                    getEntityTree()->recurseTreeWithOperator(&op);
                }
            } else {
                ++itemItr;
                if (expiry < _nextOwnerlessExpiry) {
                    _nextOwnerlessExpiry = expiry;
                }
            }
        }
    }
}

void SimpleEntitySimulation::addEntityInternal(EntityItemPointer entity) {
    EntitySimulation::addEntityInternal(entity);
    if (!entity->getSimulatorID().isNull()) {
        QMutexLocker lock(&_mutex);
        _entitiesWithSimulationOwner.insert(entity);
    } else if (entity->getDynamic() && entity->hasLocalVelocity()) {
        QMutexLocker lock(&_mutex);
        _entitiesThatNeedSimulationOwner.insert(entity);
        quint64 expiry = entity->getLastChangedOnServer() + MAX_OWNERLESS_PERIOD;
        if (expiry < _nextOwnerlessExpiry) {
            _nextOwnerlessExpiry = expiry;
        }
    }
}

void SimpleEntitySimulation::removeEntityInternal(EntityItemPointer entity) {
    EntitySimulation::removeEntityInternal(entity);
    QMutexLocker lock(&_mutex);
    _entitiesWithSimulationOwner.remove(entity);
    _entitiesThatNeedSimulationOwner.remove(entity);
}

void SimpleEntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    EntitySimulation::changeEntityInternal(entity);
    if (entity->getSimulatorID().isNull()) {
        QMutexLocker lock(&_mutex);
        _entitiesWithSimulationOwner.remove(entity);
        if (entity->getDynamic() && entity->hasLocalVelocity()) {
            _entitiesThatNeedSimulationOwner.insert(entity);
            quint64 expiry = entity->getLastChangedOnServer() + MAX_OWNERLESS_PERIOD;
            if (expiry < _nextOwnerlessExpiry) {
                _nextOwnerlessExpiry = expiry;
            }
        }
    } else {
        QMutexLocker lock(&_mutex);
        _entitiesWithSimulationOwner.insert(entity);
        _entitiesThatNeedSimulationOwner.remove(entity);
    }
    entity->clearDirtyFlags();
}

void SimpleEntitySimulation::clearEntitiesInternal() {
    QMutexLocker lock(&_mutex);
    _entitiesWithSimulationOwner.clear();
    _entitiesThatNeedSimulationOwner.clear();
}

void SimpleEntitySimulation::sortEntitiesThatMoved() {
    SetOfEntities::iterator itemItr = _entitiesToSort.begin();
    while (itemItr != _entitiesToSort.end()) {
        EntityItemPointer entity = *itemItr;
        entity->checkAndMaybeUpdateQueryAACube();
        ++itemItr;
    }
    EntitySimulation::sortEntitiesThatMoved();
}
