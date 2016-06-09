//
//  EntitySimulation.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AACube.h>

#include "EntitySimulation.h"
#include "EntitiesLogging.h"
#include "MovingEntitiesOperator.h"

void EntitySimulation::setEntityTree(EntityTreePointer tree) {
    if (_entityTree && _entityTree != tree) {
        _mortalEntities.clear();
        _nextExpiry = quint64(-1);
        _entitiesToUpdate.clear();
        _entitiesToSort.clear();
        _simpleKinematicEntities.clear();
    }
    _entityTree = tree;
}

void EntitySimulation::updateEntities() {
    QMutexLocker lock(&_mutex);
    quint64 now = usecTimestampNow();

    // these methods may accumulate entries in _entitiesToBeDeleted
    expireMortalEntities(now);
    callUpdateOnEntitiesThatNeedIt(now);
    moveSimpleKinematics(now);
    updateEntitiesInternal(now);
    PerformanceTimer perfTimer("sortingEntities");
    sortEntitiesThatMoved();
}

void EntitySimulation::takeEntitiesToDelete(VectorOfEntities& entitiesToDelete) {
    QMutexLocker lock(&_mutex);
    for (auto entity : _entitiesToDelete) {
        // push this entity onto the external list
        entitiesToDelete.push_back(entity);
    }
    _entitiesToDelete.clear();
}

void EntitySimulation::removeEntityInternal(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    // remove from all internal lists except _entitiesToDelete
    _mortalEntities.remove(entity);
    _entitiesToUpdate.remove(entity);
    _entitiesToSort.remove(entity);
    _simpleKinematicEntities.remove(entity);
    _allEntities.remove(entity);
    entity->setSimulated(false);
}

void EntitySimulation::prepareEntityForDelete(EntityItemPointer entity) {
    assert(entity);
    assert(entity->isDead());
    if (entity->isSimulated()) {
        QMutexLocker lock(&_mutex);
        entity->clearActions(getThisPointer());
        removeEntityInternal(entity);
        _entitiesToDelete.insert(entity);
    }
}

void EntitySimulation::addEntityInternal(EntityItemPointer entity) {
    if (entity->isMovingRelativeToParent() && !entity->getPhysicsInfo()) {
        QMutexLocker lock(&_mutex);
        _simpleKinematicEntities.insert(entity);
        entity->setLastSimulated(usecTimestampNow());
    }
}

void EntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    if (entity->isMovingRelativeToParent() && !entity->getPhysicsInfo()) {
        int numKinematicEntities = _simpleKinematicEntities.size();
        _simpleKinematicEntities.insert(entity);
        if (numKinematicEntities != _simpleKinematicEntities.size()) {
            entity->setLastSimulated(usecTimestampNow());
        }
    } else {
        _simpleKinematicEntities.remove(entity);
    }
}

// protected
void EntitySimulation::expireMortalEntities(const quint64& now) {
    if (now > _nextExpiry) {
        // only search for expired entities if we expect to find one
        _nextExpiry = quint64(-1);
        QMutexLocker lock(&_mutex);
        SetOfEntities::iterator itemItr = _mortalEntities.begin();
        while (itemItr != _mortalEntities.end()) {
            EntityItemPointer entity = *itemItr;
            quint64 expiry = entity->getExpiry();
            if (expiry < now) {
                itemItr = _mortalEntities.erase(itemItr);
                entity->die();
                prepareEntityForDelete(entity);
            } else {
                if (expiry < _nextExpiry) {
                    // remember the smallest _nextExpiry so we know when to start the next search
                    _nextExpiry = expiry;
                }
                ++itemItr;
            }
        }
        if (_mortalEntities.size() < 1) {
            _nextExpiry = -1;
        }
    }
}

// protected
void EntitySimulation::callUpdateOnEntitiesThatNeedIt(const quint64& now) {
    PerformanceTimer perfTimer("updatingEntities");
    QMutexLocker lock(&_mutex);
    SetOfEntities::iterator itemItr = _entitiesToUpdate.begin();
    while (itemItr != _entitiesToUpdate.end()) {
        EntityItemPointer entity = *itemItr;
        // TODO: catch transition from needing update to not as a "change"
        // so we don't have to scan for it here.
        if (!entity->needsToCallUpdate()) {
            itemItr = _entitiesToUpdate.erase(itemItr);
        } else {
            entity->update(now);
            ++itemItr;
        }
    }
}

// protected
void EntitySimulation::sortEntitiesThatMoved() {
    // NOTE: this is only for entities that have been moved by THIS EntitySimulation.
    // External changes to entity position/shape are expected to be sorted outside of the EntitySimulation.
    MovingEntitiesOperator moveOperator(_entityTree);
    AACube domainBounds(glm::vec3((float)-HALF_TREE_SCALE), (float)TREE_SCALE);
    SetOfEntities::iterator itemItr = _entitiesToSort.begin();
    while (itemItr != _entitiesToSort.end()) {
        EntityItemPointer entity = *itemItr;
        // check to see if this movement has sent the entity outside of the domain.
        bool success;
        AACube newCube = entity->getQueryAACube(success);
        if (success && !domainBounds.touches(newCube)) {
            qCDebug(entities) << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            itemItr = _entitiesToSort.erase(itemItr);
            entity->die();
            prepareEntityForDelete(entity);
        } else {
            moveOperator.addEntityToMoveList(entity, newCube);
            ++itemItr;
        }
    }
    if (moveOperator.hasMovingEntities()) {
        PerformanceTimer perfTimer("recurseTreeWithOperator");
        _entityTree->recurseTreeWithOperator(&moveOperator);
    }

    _entitiesToSort.clear();
}

void EntitySimulation::addEntity(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    assert(entity);
    entity->deserializeActions();
    if (entity->isMortal()) {
        _mortalEntities.insert(entity);
        quint64 expiry = entity->getExpiry();
        if (expiry < _nextExpiry) {
            _nextExpiry = expiry;
        }
    }
    if (entity->needsToCallUpdate()) {
        _entitiesToUpdate.insert(entity);
    }
    addEntityInternal(entity);

    _allEntities.insert(entity);
    entity->setSimulated(true);

    // DirtyFlags are used to signal changes to entities that have already been added,
    // so we can clear them for this entity which has just been added.
    entity->clearDirtyFlags();
}

void EntitySimulation::changeEntity(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    assert(entity);
    if (!entity->isSimulated()) {
        // This entity was either never added to the simulation or has been removed
        // (probably for pending delete), so we don't want to keep a pointer to it
        // on any internal lists.
        return;
    }

    // Although it is not the responsibility of the EntitySimulation to sort the tree for EXTERNAL changes
    // it IS responsibile for triggering deletes for entities that leave the bounds of the domain, hence
    // we must check for that case here, however we rely on the change event to have set DIRTY_POSITION flag.
    bool wasRemoved = false;
    uint32_t dirtyFlags = entity->getDirtyFlags();
    if (dirtyFlags & Simulation::DIRTY_POSITION) {
        AACube domainBounds(glm::vec3((float)-HALF_TREE_SCALE), (float)TREE_SCALE);
        bool success;
        AACube newCube = entity->getQueryAACube(success);
        if (success && !domainBounds.touches(newCube)) {
            qCDebug(entities) << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            entity->die();
            prepareEntityForDelete(entity);
            wasRemoved = true;
        }
    }
    if (!wasRemoved) {
        if (dirtyFlags & Simulation::DIRTY_LIFETIME) {
            if (entity->isMortal()) {
                _mortalEntities.insert(entity);
                quint64 expiry = entity->getExpiry();
                if (expiry < _nextExpiry) {
                    _nextExpiry = expiry;
                }
            } else {
                _mortalEntities.remove(entity);
            }
            entity->clearDirtyFlags(Simulation::DIRTY_LIFETIME);
        }
        if (entity->needsToCallUpdate()) {
            _entitiesToUpdate.insert(entity);
        } else {
            _entitiesToUpdate.remove(entity);
        }
        changeEntityInternal(entity);
    }
}

void EntitySimulation::clearEntities() {
    QMutexLocker lock(&_mutex);
    _mortalEntities.clear();
    _nextExpiry = quint64(-1);
    _entitiesToUpdate.clear();
    _entitiesToSort.clear();
    _simpleKinematicEntities.clear();

    clearEntitiesInternal();

    for (auto entity : _allEntities) {
        entity->setSimulated(false);
        entity->die();
    }
    _allEntities.clear();
    _entitiesToDelete.clear();
}

void EntitySimulation::moveSimpleKinematics(const quint64& now) {
    SetOfEntities::iterator itemItr = _simpleKinematicEntities.begin();
    while (itemItr != _simpleKinematicEntities.end()) {
        EntityItemPointer entity = *itemItr;

        // The entity-server doesn't know where avatars are, so don't attempt to do simple extrapolation for
        // children of avatars.  See related code in EntityMotionState::remoteSimulationOutOfSync.
        bool ancestryIsKnown;
        entity->getMaximumAACube(ancestryIsKnown);
        bool hasAvatarAncestor = entity->hasAncestorOfType(NestableType::Avatar);

        if (entity->isMovingRelativeToParent() && !entity->getPhysicsInfo() && ancestryIsKnown && !hasAvatarAncestor) {
            entity->simulate(now);
            _entitiesToSort.insert(entity);
            ++itemItr;
        } else {
            // the entity is no longer non-physical-kinematic
            itemItr = _simpleKinematicEntities.erase(itemItr);
        }
    }
}

void EntitySimulation::addAction(EntityActionPointer action) {
    QMutexLocker lock(&_mutex);
    _actionsToAdd += action;
}

void EntitySimulation::removeAction(const QUuid actionID) {
    QMutexLocker lock(&_mutex);
    _actionsToRemove += actionID;
}

void EntitySimulation::removeActions(QList<QUuid> actionIDsToRemove) {
    QMutexLocker lock(&_mutex);
    foreach(QUuid uuid, actionIDsToRemove) {
        _actionsToRemove.insert(uuid);
    }
}

void EntitySimulation::applyActionChanges() {
    QMutexLocker lock(&_mutex);
    _actionsToAdd.clear();
    _actionsToRemove.clear();
}
