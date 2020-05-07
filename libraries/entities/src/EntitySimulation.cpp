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

#include "EntitySimulation.h"

#include <AACube.h>
#include <Profile.h>

#include "EntitiesLogging.h"
#include "MovingEntitiesOperator.h"

void EntitySimulation::setEntityTree(EntityTreePointer tree) {
    if (_entityTree && _entityTree != tree) {
        _entitiesToSort.clear();
        _simpleKinematicEntities.clear();
        _changedEntities.clear();
        _entitiesToUpdate.clear();
        _mortalEntities.clear();
        _nextExpiry = std::numeric_limits<uint64_t>::max();
    }
    _entityTree = tree;
}

void EntitySimulation::updateEntities() {
    PerformanceTimer perfTimer("EntitySimulation::updateEntities");
    QMutexLocker lock(&_mutex);
    uint64_t now = usecTimestampNow();

    // these methods may accumulate entries in _entitiesToBeDeleted
    expireMortalEntities(now);
    callUpdateOnEntitiesThatNeedIt(now);
    moveSimpleKinematics(now);
    sortEntitiesThatMoved();
    processDeadEntities();
}

void EntitySimulation::removeEntityFromInternalLists(EntityItemPointer entity) {
    // protected: _mutex lock is guaranteed
    // remove from all internal lists except _deadEntitiesToRemoveFromTree
    _entitiesToSort.remove(entity);
    _simpleKinematicEntities.remove(entity);
    _allEntities.remove(entity);
    _entitiesToUpdate.remove(entity);
    _mortalEntities.remove(entity);
    entity->setSimulated(false);
}

void EntitySimulation::prepareEntityForDelete(EntityItemPointer entity) {
    assert(entity);
    assert(entity->isDead());
    if (entity->isSimulated()) {
        QMutexLocker lock(&_mutex);
        removeEntityFromInternalLists(entity);
        if (entity->getElement()) {
            _deadEntitiesToRemoveFromTree.insert(entity);
            _entityTree->cleanupCloneIDs(entity->getEntityItemID());
        }
    }
}

// protected
void EntitySimulation::expireMortalEntities(uint64_t now) {
    if (now > _nextExpiry) {
        PROFILE_RANGE_EX(simulation_physics, "ExpireMortals", 0xffff00ff, (uint64_t)_mortalEntities.size());
        // only search for expired entities if we expect to find one
        _nextExpiry = std::numeric_limits<uint64_t>::max();
        QMutexLocker lock(&_mutex);
        SetOfEntities::iterator itemItr = _mortalEntities.begin();
        while (itemItr != _mortalEntities.end()) {
            EntityItemPointer entity = *itemItr;
            uint64_t expiry = entity->getExpiry();
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
void EntitySimulation::callUpdateOnEntitiesThatNeedIt(uint64_t now) {
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
    PROFILE_RANGE_EX(simulation_physics, "SortTree", 0xffff00ff, (uint64_t)_entitiesToSort.size());
    // NOTE: this is only for entities that have been moved by THIS EntitySimulation.
    // External changes to entity position/shape are expected to be sorted outside of the EntitySimulation.
    MovingEntitiesOperator moveOperator;
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

void EntitySimulation::addEntityToInternalLists(EntityItemPointer entity) {
    // protected: _mutex lock is guaranteed
    if (entity->isMortal()) {
        _mortalEntities.insert(entity);
        uint64_t expiry = entity->getExpiry();
        if (expiry < _nextExpiry) {
            _nextExpiry = expiry;
        }
    }
    if (entity->needsToCallUpdate()) {
        _entitiesToUpdate.insert(entity);
    }
    _allEntities.insert(entity);
    entity->setSimulated(true);
}

void EntitySimulation::addEntity(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    assert(entity);
    addEntityToInternalLists(entity);

    // DirtyFlags are used to signal changes to entities that have already been added,
    // so we can clear them for this entity which has just been added.
    entity->clearDirtyFlags();
}

void EntitySimulation::changeEntity(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    assert(entity);
    _changedEntities.insert(entity);
}

void EntitySimulation::processChangedEntities() {
    QMutexLocker lock(&_mutex);
    PROFILE_RANGE_EX(simulation_physics, "processChangedEntities", 0xffff00ff, (uint64_t)_changedEntities.size());
    for (auto& entity : _changedEntities) {
        if (entity->isSimulated()) {
            processChangedEntity(entity);
        }
    }
    _changedEntities.clear();
}

void EntitySimulation::processChangedEntity(const EntityItemPointer& entity) {
    uint32_t dirtyFlags = entity->getDirtyFlags();

    if (dirtyFlags & (Simulation::DIRTY_LIFETIME | Simulation::DIRTY_UPDATEABLE)) {
        if (dirtyFlags & Simulation::DIRTY_LIFETIME) {
            if (entity->isMortal()) {
                _mortalEntities.insert(entity);
                uint64_t expiry = entity->getExpiry();
                if (expiry < _nextExpiry) {
                    _nextExpiry = expiry;
                }
            } else {
                _mortalEntities.remove(entity);
            }
        }
        if (dirtyFlags & Simulation::DIRTY_UPDATEABLE) {
            if (entity->needsToCallUpdate()) {
                _entitiesToUpdate.insert(entity);
            } else {
                _entitiesToUpdate.remove(entity);
            }
        }
        entity->clearDirtyFlags(Simulation::DIRTY_LIFETIME | Simulation::DIRTY_UPDATEABLE);
    }
}

void EntitySimulation::clearEntities() {
    QMutexLocker lock(&_mutex);
    _entitiesToSort.clear();
    _simpleKinematicEntities.clear();
    _changedEntities.clear();
    _allEntities.clear();
    _deadEntitiesToRemoveFromTree.clear();
    _entitiesToUpdate.clear();
    _mortalEntities.clear();
    _nextExpiry = std::numeric_limits<uint64_t>::max();
}

void EntitySimulation::moveSimpleKinematics(uint64_t now) {
    PROFILE_RANGE_EX(simulation_physics, "MoveSimples", 0xffff00ff, (uint64_t)_simpleKinematicEntities.size());
    SetOfEntities::iterator itemItr = _simpleKinematicEntities.begin();
    while (itemItr != _simpleKinematicEntities.end()) {
        EntityItemPointer entity = *itemItr;

        // The entity-server doesn't know where avatars are, so don't attempt to do simple extrapolation for
        // children of avatars.  See related code in EntityMotionState::remoteSimulationOutOfSync.
        bool ancestryIsKnown;
        entity->getMaximumAACube(ancestryIsKnown);
        bool hasAvatarAncestor = entity->hasAncestorOfType(NestableType::Avatar);

        bool isMoving = entity->isMovingRelativeToParent();
        if (isMoving && !entity->getPhysicsInfo() && ancestryIsKnown && !hasAvatarAncestor) {
            entity->simulate(now);
            if (ancestryIsKnown && !hasAvatarAncestor) {
                entity->updateQueryAACube();
            }
            _entitiesToSort.insert(entity);
            ++itemItr;
        } else {
            if (!isMoving && ancestryIsKnown && !hasAvatarAncestor) {
                // HACK: This catches most cases where the entity's QueryAACube (and spatial sorting in the EntityTree)
                // would otherwise be out of date at conclusion of its "unowned" simpleKinematicMotion.
                entity->updateQueryAACube();
                _entitiesToSort.insert(entity);
            }
            // the entity is no longer non-physical-kinematic
            itemItr = _simpleKinematicEntities.erase(itemItr);
        }
    }
}

void EntitySimulation::processDeadEntities() {
    if (_deadEntitiesToRemoveFromTree.empty()) {
        return;
    }
    std::vector<EntityItemPointer> entitiesToDeleteImmediately;
    entitiesToDeleteImmediately.reserve(_deadEntitiesToRemoveFromTree.size());
    QUuid nullSessionID;
    foreach (auto entity, _deadEntitiesToRemoveFromTree) {
        entitiesToDeleteImmediately.push_back(entity);
        entity->collectChildrenForDelete(entitiesToDeleteImmediately, nullSessionID);
    }
    if (_entityTree) {
        _entityTree->deleteEntitiesByPointer(entitiesToDeleteImmediately);
    }
    _deadEntitiesToRemoveFromTree.clear();
}
