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
            EntityMotionState* state = static_cast<EntityMotionState*>(*stateItr);
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
    _tempSet.clear();
    for (auto entityItr : _pendingRemoves) {
        EntityItem* entity = *entityItr;
        _physicalEntities.remove(entity);
        _pendingAdds.remove(entity);
        _pendingChanges.remove(entity);
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _tempSet.push_back(motionState);
        }
    }
    _pendingRemoves.clear();
    return _tempSet;
}

VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToAdd() {
    _tempSet.clear();
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
        _tempSet.push_back(motionState);
    }
    return _tempSet;
}

VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToChange() {
    _tempSet.clear();
    for (auto entityItr : _pendingChanges) {
        EntityItem* entity = *entityItr;
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _tempSet.push_back(motionState);
        }
    }
    _pendingChanges.clear();
    return _tempSet;
}

void PhysicalEntitySimulation::handleOutgoingChanges(VectorOfMotionStates& motionStates) {
    for (auto stateItr : motionStates) {
        ObjectMotionState* state = *stateItr;
        if (state->getType() == MOTION_STATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            _outgoingChanges.insert(entityState);
            // BOOKMARK TODO: 
            // * process _outgoingChanges.  
            // * move entityUpdate stuff out of EntityMotionState::setWorldTransform() and into new function.
            // * lock entityTree in new function
        }
    }
    sendOutgoingPackets();
}

void PhysicalEntitySimulation::bump(EntityItem* bumpEntity) {
}

/* useful CRUFT 
void PhysicalEntitySimulation::relayIncomingChangesToSimulation() {
    BT_PROFILE("incomingChanges");
    // process incoming changes
    SetOfMotionStates::iterator stateItr = _incomingChanges.begin();
    while (stateItr != _incomingChanges.end()) {
        ObjectMotionState* motionState = *stateItr;
        ++stateItr;
        uint32_t flags = motionState->getIncomingDirtyFlags() & DIRTY_PHYSICS_FLAGS;

        bool removeMotionState = false;
        btRigidBody* body = motionState->getRigidBody();
        if (body) {
            if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
                // a HARD update requires the body be pulled out of physics engine, changed, then reinserted
                // but it also handles all EASY changes
                bool success = updateObjectHard(body, motionState, flags);
                if (!success) {
                    // NOTE: since updateObjectHard() failed we know that motionState has been removed 
                    // from simulation and body has been deleted.  Depending on what else has changed
                    // we might need to remove motionState altogether...
                    if (flags & EntityItem::DIRTY_VELOCITY) {
                        motionState->updateKinematicState(_numSubsteps);
                        if (motionState->isKinematic()) {
                            // all is NOT lost, we still need to move this object around kinematically
                            _nonPhysicalKinematicEntities.insert(motionState);
                        } else {
                            // no need to keep motionState around
                            removeMotionState = true;
                        }
                    } else {
                         // no need to keep motionState around
                         removeMotionState = true;
                    }
                }
            } else if (flags) {
                // an EASY update does NOT require that the body be pulled out of physics engine
                // hence the MotionState has all the knowledge and authority to perform the update.
                motionState->updateObjectEasy(flags, _numSubsteps);
            }
            if (flags & (EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY)) {
                motionState->resetMeasuredBodyAcceleration();
            }
        } else {
            // the only way we should ever get here (motionState exists but no body) is when the object
            // is undergoing non-physical kinematic motion.
            assert(_nonPhysicalKinematicEntities.contains(motionState));

            // it is possible that the changes are such that the object can now be added to the physical simulation
            if (flags & EntityItem::DIRTY_SHAPE) {
                ShapeInfo shapeInfo;
                motionState->computeObjectShapeInfo(shapeInfo);
                btCollisionShape* shape = _shapeManager.getShape(shapeInfo);
                if (shape) {
                    addObject(shapeInfo, shape, motionState);
                    _nonPhysicalKinematicEntities.remove(motionState);
                } else if (flags & EntityItem::DIRTY_VELOCITY) {
                    // although we couldn't add the object to the simulation, might need to update kinematic motion...
                    motionState->updateKinematicState(_numSubsteps);
                    if (!motionState->isKinematic()) {
                        _nonPhysicalKinematicEntities.remove(motionState);
                        removeMotionState = true;
                    }
                }
            } else if (flags & EntityItem::DIRTY_VELOCITY) {
                // although we still can't add to physics simulation, might need to update kinematic motion...
                motionState->updateKinematicState(_numSubsteps);
                if (!motionState->isKinematic()) {
                    _nonPhysicalKinematicEntities.remove(motionState);
                    removeMotionState = true;
                }
            }
        }
        if (removeMotionState) {
            // if we get here then there is no need to keep this motionState around (no physics or kinematics)
            _outgoingPackets.remove(motionState);
            if (motionState->getType() == MOTION_STATE_TYPE_ENTITY) {
                _physicalEntities.remove(static_cast<EntityMotionState*>(motionState));
            }
            // NOTE: motionState will clean up its own backpointers in the Object
            delete motionState;
            continue;
        }

        // NOTE: the grand order of operations is:
        // (1) relay incoming changes
        // (2) step simulation
        // (3) synchronize outgoing motion states
        // (4) send outgoing packets
        //
        // We're in the middle of step (1) hence incoming changes should trump corresponding 
        // outgoing changes at this point.
        motionState->clearOutgoingPacketFlags(flags); // clear outgoing flags that were trumped
        motionState->clearIncomingDirtyFlags(flags);  // clear incoming flags that were processed
    }
    _incomingChanges.clear();
}

{
    // TODO: re-enable non-physical-kinematics 
    // step non-physical kinematic objects
    SetOfMotionStates::iterator stateItr = _nonPhysicalKinematicEntities.begin();
    // TODO?: need to occasionally scan for stopped non-physical kinematics objects
    while (stateItr != _nonPhysicalKinematicEntities.end()) {
        ObjectMotionState* motionState = *stateItr;
        motionState->stepKinematicSimulation(now);
        ++stateItr;
    }
}

void PhysicalEntitySimulation::stepNonPhysicalKinematics(const quint64& now) {
    SetOfMotionStates::iterator stateItr = _nonPhysicalKinematicEntities.begin();
    // TODO?: need to occasionally scan for stopped non-physical kinematics objects
    while (stateItr != _nonPhysicalKinematicEntities.end()) {
        ObjectMotionState* motionState = *stateItr;
        motionState->stepKinematicSimulation(now);
        ++stateItr;
    }
}
*/
