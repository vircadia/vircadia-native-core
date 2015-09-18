//
//  PhysicalEntitySimulation.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.04.27
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"
#include "ShapeManager.h"

#include "PhysicalEntitySimulation.h"

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
    assert(entity);
    if (entity->shouldBePhysical()) { 
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (!motionState) {
            _pendingAdds.insert(entity);
        }
    } else if (entity->isMoving()) {
        _simpleKinematicEntities.insert(entity);
    }
}

void PhysicalEntitySimulation::removeEntityInternal(EntityItemPointer entity) {
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    if (motionState) {
        motionState->clearObjectBackPointer();
        entity->setPhysicsInfo(nullptr);
        _pendingRemoves.insert(motionState);
        _outgoingChanges.remove(motionState);
    }
    _pendingAdds.remove(entity);
}

void PhysicalEntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    assert(entity);
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    if (motionState) {
        if (!entity->shouldBePhysical()) {
            // the entity should be removed from the physical simulation
            _pendingChanges.remove(motionState);
            _physicalObjects.remove(motionState);
            _pendingRemoves.insert(motionState);
            _outgoingChanges.remove(motionState);
            if (entity->isMoving()) {
                _simpleKinematicEntities.insert(entity);
            }
        } else {
            _pendingChanges.insert(motionState);
        }
    } else if (entity->shouldBePhysical()) {
        // The intent is for this object to be in the PhysicsEngine, but it has no MotionState yet.
        // Perhaps it's shape has changed and it can now be added?
        _pendingAdds.insert(entity);
        _simpleKinematicEntities.remove(entity); // just in case it's non-physical-kinematic
    } else if (entity->isMoving()) {
        _simpleKinematicEntities.insert(entity);
    } else {
        _simpleKinematicEntities.remove(entity); // just in case it's non-physical-kinematic
    }
}

void PhysicalEntitySimulation::clearEntitiesInternal() {
    // TODO: we should probably wait to lock the _physicsEngine so we don't mess up data structures
    // while it is in the middle of a simulation step.  As it is, we're probably in shutdown mode
    // anyway, so maybe the simulation was already properly shutdown?  Cross our fingers...

    // first disconnect each MotionStates from its Entity
    for (auto stateItr : _physicalObjects) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(&(*stateItr));
        EntityItemPointer entity = motionState->getEntity();
        if (entity) {
            entity->setPhysicsInfo(nullptr);
        }
        motionState->clearObjectBackPointer();
    }

    // then delete the objects (aka MotionStates)
    _physicsEngine->deleteObjects(_physicalObjects);

    // finally clear all lists (which now have only dangling pointers)
    _physicalObjects.clear();
    _pendingRemoves.clear();
    _pendingAdds.clear();
    _pendingChanges.clear();
}
// end EntitySimulation overrides


void PhysicalEntitySimulation::getObjectsToDelete(VectorOfMotionStates& result) {
    result.clear();
    QMutexLocker lock(&_mutex);
    for (auto stateItr : _pendingRemoves) {
        EntityMotionState* motionState = &(*stateItr);
        _pendingChanges.remove(motionState);
        _physicalObjects.remove(motionState);

        EntityItemPointer entity = motionState->getEntity();
        if (entity) {
            _pendingAdds.remove(entity);
            entity->setPhysicsInfo(nullptr);
            motionState->clearObjectBackPointer();
        }
        result.push_back(motionState);
    }
    _pendingRemoves.clear();
}

void PhysicalEntitySimulation::getObjectsToAdd(VectorOfMotionStates& result) {
    result.clear();
    QMutexLocker lock(&_mutex);
    SetOfEntities::iterator entityItr = _pendingAdds.begin();
    while (entityItr != _pendingAdds.end()) {
        EntityItemPointer entity = *entityItr;
        assert(!entity->getPhysicsInfo());
        if (!entity->shouldBePhysical()) {
            // this entity should no longer be on the internal _pendingAdds
            entityItr = _pendingAdds.erase(entityItr);
            if (entity->isMoving()) {
                _simpleKinematicEntities.insert(entity);
            }
        } else if (entity->isReadyToComputeShape()) {
            ShapeInfo shapeInfo;
            entity->computeShapeInfo(shapeInfo);
            btCollisionShape* shape = ObjectMotionState::getShapeManager()->getShape(shapeInfo);
            if (shape) {
                EntityMotionState* motionState = new EntityMotionState(shape, entity);
                entity->setPhysicsInfo(static_cast<void*>(motionState));
                _physicalObjects.insert(motionState);
                result.push_back(motionState);
                entityItr = _pendingAdds.erase(entityItr);
            } else {
                //qDebug() << "Warning!  Failed to generate new shape for entity." << entity->getName();
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

void PhysicalEntitySimulation::handleOutgoingChanges(const VectorOfMotionStates& motionStates, const QUuid& sessionID) {
    QMutexLocker lock(&_mutex);
    // walk the motionStates looking for those that correspond to entities
    for (auto stateItr : motionStates) {
        ObjectMotionState* state = &(*stateItr);
        if (state && state->getType() == MOTIONSTATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            EntityItemPointer entity = entityState->getEntity();
            if (entity) {
                if (entityState->isCandidateForOwnership(sessionID)) {
                    _outgoingChanges.insert(entityState);
                }
                _entitiesToSort.insert(entityState->getEntity());
            }
        }
    }

    uint32_t numSubsteps = _physicsEngine->getNumSubsteps();
    if (_lastStepSendPackets != numSubsteps) {
        _lastStepSendPackets = numSubsteps;

        if (sessionID.isNull()) {
            // usually don't get here, but if so --> nothing to do
            _outgoingChanges.clear();
            return;
        }

        // send outgoing packets
        QSet<EntityMotionState*>::iterator stateItr = _outgoingChanges.begin();
        while (stateItr != _outgoingChanges.end()) {
            EntityMotionState* state = *stateItr;
            if (!state->isCandidateForOwnership(sessionID)) {
                stateItr = _outgoingChanges.erase(stateItr);
            } else if (state->shouldSendUpdate(numSubsteps, sessionID)) {
                state->sendUpdate(_entityPacketSender, sessionID, numSubsteps);
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


void PhysicalEntitySimulation::addAction(EntityActionPointer action) {
    if (_physicsEngine) {
        // FIXME put fine grain locking into _physicsEngine
        {
            QMutexLocker lock(&_mutex);
            const QUuid& actionID = action->getID();
            if (_physicsEngine->getActionByID(actionID)) {
                qDebug() << "warning -- PhysicalEntitySimulation::addAction -- adding an action that was already in _physicsEngine";
            }
        }
        EntitySimulation::addAction(action);
    }
}

void PhysicalEntitySimulation::applyActionChanges() {
    if (_physicsEngine) {
        // FIXME put fine grain locking into _physicsEngine
        QMutexLocker lock(&_mutex);
        foreach(QUuid actionToRemove, _actionsToRemove) {
            _physicsEngine->removeAction(actionToRemove);
        }
        foreach (EntityActionPointer actionToAdd, _actionsToAdd) {
            if (!_actionsToRemove.contains(actionToAdd->getID())) {
                _physicsEngine->addAction(actionToAdd);
            }
        }
    }
    EntitySimulation::applyActionChanges();
}
