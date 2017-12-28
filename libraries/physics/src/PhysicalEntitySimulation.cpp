//
//  PhysicalEntitySimulation.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.04.27
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "PhysicalEntitySimulation.h"

#include <Profile.h>

#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"
#include "ShapeManager.h"


PhysicalEntitySimulation::PhysicalEntitySimulation() {
}

PhysicalEntitySimulation::~PhysicalEntitySimulation() {
}

void PhysicalEntitySimulation::init(
        EntityTreePointer tree,
        PhysicsEnginePointer physicsEngine,
        EntityEditPacketSender* packetSender) {
    assert(tree);
    setEntityTree(tree);

    assert(physicsEngine);
    _physicsEngine = physicsEngine;

    assert(packetSender);
    _entityPacketSender = packetSender;
}

// begin EntitySimulation overrides
void PhysicalEntitySimulation::updateEntitiesInternal(const quint64& now) {
    // Do nothing here because the "internal" update the PhysicsEngine::stepSimualtion() which is done elsewhere.
}

void PhysicalEntitySimulation::addEntityInternal(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    assert(entity);
    assert(!entity->isDead());
    if (entity->shouldBePhysical()) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (!motionState) {
            _entitiesToAddToPhysics.insert(entity);
        }
    } else if (entity->isMovingRelativeToParent()) {
        _simpleKinematicEntities.insert(entity);
    }
}

void PhysicalEntitySimulation::removeEntityInternal(EntityItemPointer entity) {
    if (entity->isSimulated()) {
        EntitySimulation::removeEntityInternal(entity);
        QMutexLocker lock(&_mutex);
        _entitiesToAddToPhysics.remove(entity);

        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _outgoingChanges.remove(motionState);
            _entitiesToRemoveFromPhysics.insert(entity);
        } else {
            _entitiesToDelete.insert(entity);
        }
    }
}

void PhysicalEntitySimulation::takeEntitiesToDelete(VectorOfEntities& entitiesToDelete) {
    QMutexLocker lock(&_mutex);
    for (auto entity : _entitiesToDelete) {
        // this entity is still in its tree, so we insert into the external list
        entitiesToDelete.push_back(entity);

        // Someday when we invert the entities/physics lib dependencies we can let EntityItem delete its own PhysicsInfo
        // rather than do it here
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _entitiesToRemoveFromPhysics.insert(entity);
        }
    }
    _entitiesToDelete.clear();
}

void PhysicalEntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    QMutexLocker lock(&_mutex);
    assert(entity);
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    if (motionState) {
        if (!entity->shouldBePhysical()) {
            // the entity should be removed from the physical simulation
            _pendingChanges.remove(motionState);
            _physicalObjects.remove(motionState);
            _outgoingChanges.remove(motionState);
            _entitiesToRemoveFromPhysics.insert(entity);
            if (entity->isMovingRelativeToParent()) {
                _simpleKinematicEntities.insert(entity);
            }
        } else {
            _pendingChanges.insert(motionState);
        }
    } else if (entity->shouldBePhysical()) {
        // The intent is for this object to be in the PhysicsEngine, but it has no MotionState yet.
        // Perhaps it's shape has changed and it can now be added?
        _entitiesToAddToPhysics.insert(entity);
        _simpleKinematicEntities.remove(entity); // just in case it's non-physical-kinematic
    } else if (entity->isMovingRelativeToParent()) {
        _simpleKinematicEntities.insert(entity);
    } else {
        _simpleKinematicEntities.remove(entity); // just in case it's non-physical-kinematic
    }
}

void PhysicalEntitySimulation::clearEntitiesInternal() {
    // TODO: we should probably wait to lock the _physicsEngine so we don't mess up data structures
    // while it is in the middle of a simulation step.  As it is, we're probably in shutdown mode
    // anyway, so maybe the simulation was already properly shutdown?  Cross our fingers...

    // copy everything into _entitiesToDelete
    for (auto stateItr : _physicalObjects) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(&(*stateItr));
        _entitiesToDelete.insert(motionState->getEntity());
    }

    // then remove the objects (aka MotionStates) from physics
    _physicsEngine->removeSetOfObjects(_physicalObjects);

    // delete the MotionStates
    // TODO: after we invert the entities/physics lib dependencies we will let EntityItem delete
    // its own PhysicsInfo rather than do it here
    for (auto entity : _entitiesToDelete) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            entity->setPhysicsInfo(nullptr);
            delete motionState;
        }
    }

    // finally clear all lists maintained by this class
    _physicalObjects.clear();
    _entitiesToRemoveFromPhysics.clear();
    _entitiesToRelease.clear();
    _entitiesToAddToPhysics.clear();
    _pendingChanges.clear();
    _outgoingChanges.clear();
}

// virtual
void PhysicalEntitySimulation::prepareEntityForDelete(EntityItemPointer entity) {
    assert(entity);
    assert(entity->isDead());
    entity->clearActions(getThisPointer());
    removeEntityInternal(entity);
}
// end EntitySimulation overrides

void PhysicalEntitySimulation::getObjectsToRemoveFromPhysics(VectorOfMotionStates& result) {
    result.clear();
    QMutexLocker lock(&_mutex);
    for (auto entity: _entitiesToRemoveFromPhysics) {
        // make sure it isn't on any side lists
        _entitiesToAddToPhysics.remove(entity);

        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _pendingChanges.remove(motionState);
            _outgoingChanges.remove(motionState);
            _physicalObjects.remove(motionState);
            result.push_back(motionState);
            _entitiesToRelease.insert(entity);
        }

        if (entity->isDead()) {
            _entitiesToDelete.insert(entity);
        }
    }
    _entitiesToRemoveFromPhysics.clear();
}

void PhysicalEntitySimulation::deleteObjectsRemovedFromPhysics() {
    QMutexLocker lock(&_mutex);
    for (auto entity: _entitiesToRelease) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        assert(motionState);
        entity->setPhysicsInfo(nullptr);
        delete motionState;

        if (entity->isDead()) {
            _entitiesToDelete.insert(entity);
        }
    }
    _entitiesToRelease.clear();
}

void PhysicalEntitySimulation::getObjectsToAddToPhysics(VectorOfMotionStates& result) {
    result.clear();
    QMutexLocker lock(&_mutex);
    SetOfEntities::iterator entityItr = _entitiesToAddToPhysics.begin();
    while (entityItr != _entitiesToAddToPhysics.end()) {
        EntityItemPointer entity = (*entityItr);
        assert(!entity->getPhysicsInfo());
        if (entity->isDead()) {
            prepareEntityForDelete(entity);
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
        } else if (!entity->shouldBePhysical()) {
            // this entity should no longer be on the internal _entitiesToAddToPhysics
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
            if (entity->isMovingRelativeToParent()) {
                _simpleKinematicEntities.insert(entity);
            }
        } else if (entity->isReadyToComputeShape()) {
            ShapeInfo shapeInfo;
            entity->computeShapeInfo(shapeInfo);
            int numPoints = shapeInfo.getLargestSubshapePointCount();
            if (shapeInfo.getType() == SHAPE_TYPE_COMPOUND) {
                if (numPoints > MAX_HULL_POINTS) {
                    qWarning() << "convex hull with" << numPoints
                        << "points for entity" << entity->getName()
                        << "at" << entity->getWorldPosition() << " will be reduced";
                }
            }
            btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
            if (shape) {
                EntityMotionState* motionState = new EntityMotionState(shape, entity);
                entity->setPhysicsInfo(static_cast<void*>(motionState));
                _physicalObjects.insert(motionState);
                result.push_back(motionState);
                entityItr = _entitiesToAddToPhysics.erase(entityItr);
            } else {
                //qWarning() << "Failed to generate new shape for entity." << entity->getName();
                ++entityItr;
            }
        } else {
            ++entityItr;
        }
    }
}

void PhysicalEntitySimulation::setObjectsToChange(const VectorOfMotionStates& objectsToChange) {
    QMutexLocker lock(&_mutex);
    for (auto object : objectsToChange) {
        _pendingChanges.insert(static_cast<EntityMotionState*>(object));
    }
}

void PhysicalEntitySimulation::getObjectsToChange(VectorOfMotionStates& result) {
    result.clear();
    QMutexLocker lock(&_mutex);
    for (auto stateItr : _pendingChanges) {
        EntityMotionState* motionState = &(*stateItr);
        result.push_back(motionState);
    }
    _pendingChanges.clear();
}

void PhysicalEntitySimulation::handleDeactivatedMotionStates(const VectorOfMotionStates& motionStates) {
    for (auto stateItr : motionStates) {
        ObjectMotionState* state = &(*stateItr);
        assert(state);
        if (state->getType() == MOTIONSTATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            entityState->handleDeactivation();
            EntityItemPointer entity = entityState->getEntity();
            _entitiesToSort.insert(entity);
        }
    }
}

void PhysicalEntitySimulation::handleChangedMotionStates(const VectorOfMotionStates& motionStates) {
    PROFILE_RANGE_EX(simulation_physics, "ChangedEntities", 0x00000000, (uint64_t)motionStates.size());
    QMutexLocker lock(&_mutex);

    // walk the motionStates looking for those that correspond to entities
    {
        PROFILE_RANGE_EX(simulation_physics, "Filter", 0x00000000, (uint64_t)motionStates.size());
        for (auto stateItr : motionStates) {
            ObjectMotionState* state = &(*stateItr);
            assert(state);
            if (state->getType() == MOTIONSTATE_TYPE_ENTITY) {
                EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
                EntityItemPointer entity = entityState->getEntity();
                assert(entity.get());
                if (entityState->isCandidateForOwnership()) {
                    _outgoingChanges.insert(entityState);
                }
                _entitiesToSort.insert(entity);
            }
        }
    }

    uint32_t numSubsteps = _physicsEngine->getNumSubsteps();
    if (_lastStepSendPackets != numSubsteps) {
        _lastStepSendPackets = numSubsteps;

        if (Physics::getSessionUUID().isNull()) {
            // usually don't get here, but if so --> nothing to do
            _outgoingChanges.clear();
            return;
        }

        // look for entities to prune or update
        PROFILE_RANGE_EX(simulation_physics, "Prune/Send", 0x00000000, (uint64_t)_outgoingChanges.size());
        QSet<EntityMotionState*>::iterator stateItr = _outgoingChanges.begin();
        while (stateItr != _outgoingChanges.end()) {
            EntityMotionState* state = *stateItr;
            if (!state->isCandidateForOwnership()) {
                // prune
                stateItr = _outgoingChanges.erase(stateItr);
            } else if (state->shouldSendUpdate(numSubsteps)) {
                // update
                state->sendUpdate(_entityPacketSender, numSubsteps);
                ++stateItr;
            } else {
                ++stateItr;
            }
        }
    }
}

void PhysicalEntitySimulation::handleCollisionEvents(const CollisionEvents& collisionEvents) {
    for (auto collision : collisionEvents) {
        // NOTE: The collision event is always aligned such that idA is never NULL.
        // however idB may be NULL.
        if (!collision.idB.isNull()) {
            emit entityCollisionWithEntity(collision.idA, collision.idB, collision);
        }
    }
}


void PhysicalEntitySimulation::addDynamic(EntityDynamicPointer dynamic) {
    if (_physicsEngine) {
        // FIXME put fine grain locking into _physicsEngine
        {
            QMutexLocker lock(&_mutex);
            const QUuid& dynamicID = dynamic->getID();
            if (_physicsEngine->getDynamicByID(dynamicID)) {
                qCDebug(physics) << "warning -- PhysicalEntitySimulation::addDynamic -- adding an "
                    "dynamic that was already in _physicsEngine";
            }
        }
        EntitySimulation::addDynamic(dynamic);
    }
}

void PhysicalEntitySimulation::applyDynamicChanges() {
    QList<EntityDynamicPointer> dynamicsFailedToAdd;
    if (_physicsEngine) {
        QMutexLocker lock(&_dynamicsMutex);
        foreach(QUuid dynamicToRemove, _dynamicsToRemove) {
            _physicsEngine->removeDynamic(dynamicToRemove);
        }
        foreach (EntityDynamicPointer dynamicToAdd, _dynamicsToAdd) {
            if (!_dynamicsToRemove.contains(dynamicToAdd->getID())) {
                if (!_physicsEngine->addDynamic(dynamicToAdd)) {
                    dynamicsFailedToAdd += dynamicToAdd;
                }
            }
        }
        // applyDynamicChanges will clear _dynamicsToRemove and _dynamicsToAdd
        EntitySimulation::applyDynamicChanges();
    }

    // put back the ones that couldn't yet be added
    foreach (EntityDynamicPointer dynamicFailedToAdd, dynamicsFailedToAdd) {
        addDynamic(dynamicFailedToAdd);
    }
}
