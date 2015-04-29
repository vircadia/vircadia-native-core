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

#include "PhysicalEntitySimulation.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"
#include "ShapeInfoUtil.h"

PhysicalEntitySimulation::PhysicalEntitySimulation() :
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

    assert(*physicsEngine);
    _physicsEngine = physicsEngine;

    assert(*shapeManager);
    _shapeManager = shapeManager;

    assert(packetSender);
    _entityPacketSender = packetSender;
}

// begin EntitySimulation overrides
void PhysicalEntitySimulation::updateEntitiesInternal(const quint64& now) {
    // TODO: add back non-physical kinematic objects and step them forward here
}

void PhysicalEntitySimulation::sendOutgoingPackets() {
    uint32_t numSubsteps = _physicsEngine->getNumSubsteps();
    if (_lastNumSubstepsAtUpdateInternal != numSubsteps) {
        _lastNumSubstepsAtUpdateInternal = numSubsteps;

        SetOfMotionStates::iterator stateItr = _outgoingChanges.begin();
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

void PhysicalEntitySimulation::addEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (!physicsInfo) {
        _pendingAdds.insert(entity);
    }
}

void PhysicalEntitySimulation::removeEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        _pendingRemoves.insert(motionState);
    } else {
        entity->_simulation = nullptr;
    }
}

void PhysicalEntitySimulation::deleteEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        _pendingRemoves.insert(motionState);
    } else {
        entity->_simulation = nullptr;
        delete entity;
    }
}

void PhysicalEntitySimulation::entityChangedInternal(EntityItem* entity) {
    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        _pendingChanges.insert(motionState);
    } else { 
        if (!entity->ignoreForCollisions()) {
            // The intent is for this object to be in the PhysicsEngine.  
            // Perhaps it's shape has changed and it can now be added?
            _pendingAdds.insert(entity);
        }
    }
}

void PhysicalEntitySimulation::sortEntitiesThatMovedInternal() {
    /*
    // entities that have been simulated forward (hence in the _entitiesToSort list) 
    // also need to be put in the outgoingPackets list
    QSet<EntityItem*>::iterator entityItr = _entitiesToSort.begin();
    for (auto entityItr : _entitiesToSort) {
        EntityItem* entity = *entityItr;
        void* physicsInfo = entity->getPhysicsInfo();
        assert(physicsInfo);
        // BOOKMARK XXX -- Andrew to fix this next
        _outgoingChanges.insert(static_cast<ObjectMotionState*>(physicsInfo));
    }
    */
}

void PhysicalEntitySimulation::clearEntitiesInternal() {
    // TODO: we should probably wait to lock the _physicsEngine so we don't mess up data structures
    // while it is in the middle of a simulation step.  As it is, we're probably in shutdown mode
    // anyway, so maybe the simulation was already properly shutdown?  Cross our fingers...
    SetOfMotionStates::const_iterator stateItr = _physicalEntities.begin();
    for (auto stateItr : _physicalEntities) {
        EntityMotionState motionState = static_cast<EntityMotionState*>(*stateItr);
        _physicsEngine->removeObjectFromBullet(motionState);
        EntityItem* entity = motionState->_entity;
        _entity->setPhysicsInfo(nullptr);
        delete motionState;
    }
    _physicalEntities.clear();
    _pendingRemoves.clear();
    _pendingAdds.clear();
    _pendingChanges.clear();
}
// end EntitySimulation overrides


VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToRemove() {
    _tempVector.clear();
    for (auto entityItr : _pendingRemoves) {
        EntityItem* entity = *entityItr;
        _physicalEntities.remove(entity);
        _pendingAdds.remove(entity);
        _pendingChanges.remove(entity);
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _tempVector.push_back(motionState);
        }
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

        if (entity->isReadyToComputeShape()) {
            ShapeInfo shapeInfo;
            entity->computeShapeInfo(shapeInfo);
            btCollisionShape* shape = _shapeManager->getShape(shapeInfo);
            if (shape) {
                EntityMotionState* motionState = new EntityMotionState(shape, entity);
                entity->setPhysicsInfo(static_cast<void*>(motionState));
                _physicalEntities.insert(motionState);
                entityItr = _pendingAdds.erase(entityItr);
                // TODO: figure out how to get shapeInfo (or relevant info) to PhysicsEngine during add
                //_physicsEngine->addObject(shapeInfo, motionState);
            } else {
                ++entityItr;
            }
        }
        _tempVector.push_back(motionState);
    }
    return _tempVector;
}

VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToChange() {
    _tempVector.clear();
    for (auto entityItr : _pendingChanges) {
        EntityItem* entity = *entityItr;
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _tempVector.push_back(motionState);
        }
    }
    _pendingChanges.clear();
    return _tempVector;
}

void PhysicalEntitySimulation::handleOutgoingChanges(VectorOfMotionStates& motionStates) {
    // walk the motionStates looking for those that correspond to entities
    for (auto stateItr : motionStates) {
        ObjectMotionState* state = *stateItr;
        if (state->getType() == MOTION_STATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            _outgoingChanges.insert(entityState);
            _entitiesToSort.insert(entityState->getEntityItem());
        }
    }
    sendOutgoingPackets();
}

void PhysicalEntitySimulation::bump(EntityItem* bumpEntity) {
}

