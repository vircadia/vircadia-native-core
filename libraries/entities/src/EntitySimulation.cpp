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

void EntitySimulation::setEntityTree(EntityTree* tree) {
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
    quint64 now = usecTimestampNow();

    // these methods may accumulate entries in _entitiesToBeDeleted
    expireMortalEntities(now);
    callUpdateOnEntitiesThatNeedIt(now);
    moveSimpleKinematics(now);
    updateEntitiesInternal(now);
    sortEntitiesThatMoved();
}

void EntitySimulation::getEntitiesToDelete(VectorOfEntities& entitiesToDelete) {

    for (auto entity : _entitiesToDelete) {
        // this entity is still in its tree, so we insert into the external list
        entitiesToDelete.push_back(entity);
    }
    _entitiesToDelete.clear();
}

void EntitySimulation::addEntityInternal(EntityItemPointer entity) {
    if (entity->isMoving() && !entity->getPhysicsInfo()) {
        _simpleKinematicEntities.insert(entity);
    }
}

void EntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    if (entity->isMoving() && !entity->getPhysicsInfo()) {
        _simpleKinematicEntities.insert(entity);
    } else {
        _simpleKinematicEntities.remove(entity);
    }
}

// protected
void EntitySimulation::expireMortalEntities(const quint64& now) {
    if (now > _nextExpiry) {
        // only search for expired entities if we expect to find one
        _nextExpiry = quint64(-1);
        SetOfEntities::iterator itemItr = _mortalEntities.begin();
        while (itemItr != _mortalEntities.end()) {
            EntityItemPointer entity = *itemItr;
            quint64 expiry = entity->getExpiry();
            if (expiry < now) {
                _entitiesToDelete.insert(entity);
                itemItr = _mortalEntities.erase(itemItr);
                _entitiesToUpdate.remove(entity);
                _entitiesToSort.remove(entity);
                _simpleKinematicEntities.remove(entity);
                removeEntityInternal(entity);

                _allEntities.remove(entity);
                entity->_simulated = false;
            } else {
                if (expiry < _nextExpiry) {
                    // remeber the smallest _nextExpiry so we know when to start the next search
                    _nextExpiry = expiry;
                }
                ++itemItr;
            }
        }
    }
}

// protected
void EntitySimulation::callUpdateOnEntitiesThatNeedIt(const quint64& now) {
    PerformanceTimer perfTimer("updatingEntities");
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
    PerformanceTimer perfTimer("sortingEntities");
    MovingEntitiesOperator moveOperator(_entityTree);
    AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), (float)TREE_SCALE);
    SetOfEntities::iterator itemItr = _entitiesToSort.begin();
    while (itemItr != _entitiesToSort.end()) {
        EntityItemPointer entity = *itemItr;
        // check to see if this movement has sent the entity outside of the domain.
        AACube newCube = entity->getMaximumAACube();
        if (!domainBounds.touches(newCube)) {
            qCDebug(entities) << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            _entitiesToDelete.insert(entity);
            _mortalEntities.remove(entity);
            _entitiesToUpdate.remove(entity);
            _simpleKinematicEntities.remove(entity);
            removeEntityInternal(entity);

            _allEntities.remove(entity);
            entity->_simulated = false;

            itemItr = _entitiesToSort.erase(itemItr);
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
    entity->_simulated = true;

    // DirtyFlags are used to signal changes to entities that have already been added, 
    // so we can clear them for this entity which has just been added.
    entity->clearDirtyFlags();
}

void EntitySimulation::removeEntity(EntityItemPointer entity) {
    assert(entity);
    _entitiesToUpdate.remove(entity);
    _mortalEntities.remove(entity);
    _entitiesToSort.remove(entity);
    _simpleKinematicEntities.remove(entity);
    _entitiesToDelete.remove(entity);
    removeEntityInternal(entity);

    _allEntities.remove(entity);
    entity->_simulated = false;
}

void EntitySimulation::changeEntity(EntityItemPointer entity) {
    assert(entity);
    if (!entity->_simulated) {
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
    if (dirtyFlags & EntityItem::DIRTY_POSITION) {
        AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), (float)TREE_SCALE);
        AACube newCube = entity->getMaximumAACube();
        if (!domainBounds.touches(newCube)) {
            qCDebug(entities) << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            _entitiesToDelete.insert(entity);
            _mortalEntities.remove(entity);
            _entitiesToUpdate.remove(entity);
            _entitiesToSort.remove(entity);
            _simpleKinematicEntities.remove(entity);
            removeEntityInternal(entity);
            entity->_simulated = false;
            wasRemoved = true;
        }
    }
    if (!wasRemoved) {
        if (dirtyFlags & EntityItem::DIRTY_LIFETIME) {
            if (entity->isMortal()) {
                _mortalEntities.insert(entity);
                quint64 expiry = entity->getExpiry();
                if (expiry < _nextExpiry) {
                    _nextExpiry = expiry;
                }
            } else {
                _mortalEntities.remove(entity);
            }
            entity->clearDirtyFlags(EntityItem::DIRTY_LIFETIME);
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
    _mortalEntities.clear();
    _nextExpiry = quint64(-1);
    _entitiesToUpdate.clear();
    _entitiesToSort.clear();
    _simpleKinematicEntities.clear();
    _entitiesToDelete.clear();

    clearEntitiesInternal();

    for (auto entityItr : _allEntities) {
        entityItr->_simulated = false;
    }
    _allEntities.clear();
}

void EntitySimulation::moveSimpleKinematics(const quint64& now) {
    SetOfEntities::iterator itemItr = _simpleKinematicEntities.begin();
    while (itemItr != _simpleKinematicEntities.end()) {
        EntityItemPointer entity = *itemItr;
        if (entity->isMoving() && !entity->getPhysicsInfo()) {
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
    lock();
    _actionsToAdd += action;
    unlock();
}

void EntitySimulation::removeAction(const QUuid actionID) {
    lock();
    _actionsToRemove += actionID;
    unlock();
}

void EntitySimulation::removeActions(QList<QUuid> actionIDsToRemove) {
    lock();
    _actionsToRemove += actionIDsToRemove;
    unlock();
}

void EntitySimulation::applyActionChanges() {
    lock();
    _actionsToAdd.clear();
    _actionsToRemove.clear();
    unlock();
}
