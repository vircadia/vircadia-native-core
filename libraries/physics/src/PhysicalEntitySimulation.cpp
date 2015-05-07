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

#include "EntityMotionState.h"
#include "PhysicalEntitySimulation.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"
#include "ShapeInfoUtil.h"
#include "ShapeManager.h"

PhysicalEntitySimulation::PhysicalEntitySimulation() {
}

PhysicalEntitySimulation::~PhysicalEntitySimulation() {
}

void PhysicalEntitySimulation::init(
        EntityTree* tree, 
        PhysicsEngine* physicsEngine, 
        ShapeManager* shapeManager, 
        EntityEditPacketSender* packetSender) {
    assert(tree);
    setEntityTree(tree);

    assert(physicsEngine);
    _physicsEngine = physicsEngine;

    assert(shapeManager);
    _shapeManager = shapeManager;

    assert(packetSender);
    _entityPacketSender = packetSender;
}

// begin EntitySimulation overrides
void PhysicalEntitySimulation::updateEntitiesInternal(const quint64& now) {
    // TODO: add back non-physical kinematic objects and step them forward here
}

void PhysicalEntitySimulation::addEntityInternal(EntityItem* entity) {
    assert(entity);
    if (entity->getIgnoreForCollisions() && entity->isMoving()) {
        _simpleKinematicEntities.insert(entity);
    } else {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (!motionState) {
            _pendingAdds.insert(entity);
        } else {
            // DEBUG -- Andrew to remove this after testing
            // Adding entity already in simulation? assert that this is case, 
            // since otherwise we probably have an orphaned EntityMotionState.
            assert(_physicalObjects.find(motionState) != _physicalObjects.end());
        }
    }
}

void PhysicalEntitySimulation::removeEntityInternal(EntityItem* entity) {
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    if (motionState) {
        motionState->clearEntity();    
        entity->setPhysicsInfo(nullptr);

        // NOTE: we must remove entity from _pendingAdds immediately because we've disconnected the backpointers between
        // motionState and entity and they can't be used to look up each other.  However we don't need to remove 
        // motionState from _pendingChanges at this time becuase it will be removed during getObjectsToDelete().
        _pendingAdds.remove(entity);

        _pendingRemoves.insert(motionState);
        _outgoingChanges.remove(motionState);
    }
}

void PhysicalEntitySimulation::changeEntityInternal(EntityItem* entity) {
    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    assert(entity);
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    if (motionState) {
        if (entity->getIgnoreForCollisions()) {
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
    } else if (!entity->getIgnoreForCollisions()) {
        // The intent is for this object to be in the PhysicsEngine.  
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
        EntityItem* entity = motionState->getEntity();
        if (entity) {
            entity->setPhysicsInfo(nullptr);
        }
        motionState->clearEntity();
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


VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToDelete() {
    _tempVector.clear();
    for (auto stateItr : _pendingRemoves) {
        EntityMotionState* motionState = &(*stateItr);
        _pendingChanges.remove(motionState);
        _physicalObjects.remove(motionState);

        EntityItem* entity = motionState->getEntity();
        if (entity) {
            _pendingAdds.remove(entity);
            entity->setPhysicsInfo(nullptr);
            motionState->clearEntity();
        }
        _tempVector.push_back(motionState);
    }
    _pendingRemoves.clear();
    return _tempVector;
}

VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToAdd() {
    _tempVector.clear();
    SetOfEntities::iterator entityItr = _pendingAdds.begin();
    while (entityItr != _pendingAdds.end()) {
        EntityItem* entity = *entityItr;
        assert(!entity->getPhysicsInfo());
        if (entity->getShapeType() == SHAPE_TYPE_NONE || entity->getIgnoreForCollisions()) {
            // this entity should no longer be on the internal _pendingAdds
            entityItr = _pendingAdds.erase(entityItr);
        } else if (entity->isReadyToComputeShape()) {
            ShapeInfo shapeInfo;
            entity->computeShapeInfo(shapeInfo);
            btCollisionShape* shape = _shapeManager->getShape(shapeInfo);
            if (shape) {
                EntityMotionState* motionState = new EntityMotionState(shape, entity);
                entity->setPhysicsInfo(static_cast<void*>(motionState));
                motionState->setMass(entity->computeMass());
                _physicalObjects.insert(motionState);
                _tempVector.push_back(motionState);
                entityItr = _pendingAdds.erase(entityItr);
            } else {
                qDebug() << "Warning!  Failed to generate new shape for entity." << entity->getName();
                ++entityItr;
            }
        } else {
            ++entityItr;
        }
    }
    return _tempVector;
}

VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToChange() {
    _tempVector.clear();
    for (auto stateItr : _pendingChanges) {
        EntityMotionState* motionState = &(*stateItr);
        _tempVector.push_back(motionState);
    }
    _pendingChanges.clear();
    return _tempVector;
}

void PhysicalEntitySimulation::handleOutgoingChanges(VectorOfMotionStates& motionStates) {
    // walk the motionStates looking for those that correspond to entities
    for (auto stateItr : motionStates) {
        ObjectMotionState* state = &(*stateItr);
        if (state && state->getType() == MOTION_STATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            EntityItem* entity = entityState->getEntity();
            if (entity) {
                _outgoingChanges.insert(entityState);
                _entitiesToSort.insert(entityState->getEntity());
            }
        }
    }
   
    // send outgoing packets
    uint32_t numSubsteps = _physicsEngine->getNumSubsteps();
    if (_lastStepSendPackets != numSubsteps) {
        _lastStepSendPackets = numSubsteps;

        QSet<EntityMotionState*>::iterator stateItr = _outgoingChanges.begin();
        while (stateItr != _outgoingChanges.end()) {
            EntityMotionState* state = *stateItr;
            if (state->doesNotNeedToSendUpdate()) {
                stateItr = _outgoingChanges.erase(stateItr);
            } else if (state->shouldSendUpdate(numSubsteps)) {
                state->sendUpdate(_entityPacketSender, numSubsteps);
                ++stateItr;
            } else {
                ++stateItr;
            }
        }
    }
}

void PhysicalEntitySimulation::handleCollisionEvents(CollisionEvents& collisionEvents) {
    for (auto collision : collisionEvents) {
        // NOTE: The collision event is always aligned such that idA is never NULL.
        // however idB may be NULL.
        if (!collision.idB.isNull()) {
            emit entityCollisionWithEntity(collision.idA, collision.idB, collision);
        }
    }
}

