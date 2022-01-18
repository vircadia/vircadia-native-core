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
void PhysicalEntitySimulation::addEntityToInternalLists(EntityItemPointer entity) {
    EntitySimulation::addEntityToInternalLists(entity);
    entity->deserializeActions(); // TODO: do this elsewhere
    uint8_t region = _space->getRegion(entity->getSpaceIndex());
    bool maybeShouldBePhysical = (region < workload::Region::R3 || region == workload::Region::UNKNOWN) && entity->shouldBePhysical();
    bool canBeKinematic = region <= workload::Region::R3;
    if (maybeShouldBePhysical) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            motionState->setRegion(region);
        } else {
            _entitiesToAddToPhysics.insert(entity);
        }
    } else if (canBeKinematic && entity->isMovingRelativeToParent()) {
        SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
        if (itr == _simpleKinematicEntities.end()) {
            _simpleKinematicEntities.insert(entity);
        }
    }
}

void PhysicalEntitySimulation::removeEntityFromInternalLists(EntityItemPointer entity) {
    _entitiesToAddToPhysics.remove(entity);
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    if (motionState) {
        removeOwnershipData(motionState);
        _entitiesToRemoveFromPhysics.insert(entity);
    }
    if (entity->isDead() && entity->getElement()) {
        _deadEntitiesToRemoveFromTree.insert(entity);
    }
    if (entity->isAvatarEntity()) {
        _deadAvatarEntities.insert(entity);
    }
    EntitySimulation::removeEntityFromInternalLists(entity);
}

void PhysicalEntitySimulation::removeOwnershipData(EntityMotionState* motionState) {
    assert(motionState);
    if (motionState->getOwnershipState() == EntityMotionState::OwnershipState::LocallyOwned) {
        for (uint32_t i = 0; i < _owned.size(); ++i) {
            if (_owned[i] == motionState) {
                _owned[i]->clearOwnershipState();
                _owned.remove(i);
            }
        }
    } else if (motionState->getOwnershipState() == EntityMotionState::OwnershipState::PendingBid) {
        for (uint32_t i = 0; i < _bids.size(); ++i) {
            if (_bids[i] == motionState) {
                _bids[i]->clearOwnershipState();
                _bids.remove(i);
            }
        }
    }
}

void PhysicalEntitySimulation::clearOwnershipData() {
    for (uint32_t i = 0; i < _owned.size(); ++i) {
        _owned[i]->clearOwnershipState();
    }
    _owned.clear();
    for (uint32_t i = 0; i < _bids.size(); ++i) {
        _bids[i]->clearOwnershipState();
    }
    _bids.clear();
}

void PhysicalEntitySimulation::takeDeadAvatarEntities(SetOfEntities& deadEntities) {
    _deadAvatarEntities.swap(deadEntities);
    _deadAvatarEntities.clear();
}

void PhysicalEntitySimulation::processChangedEntity(const EntityItemPointer& entity) {
    EntitySimulation::processChangedEntity(entity);

    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
    uint8_t region = _space->getRegion(entity->getSpaceIndex());
    bool shouldBePhysical = region < workload::Region::R3 && entity->shouldBePhysical();
    bool canBeKinematic = region <= workload::Region::R3;
    if (motionState) {
        if (!shouldBePhysical) {
            if (motionState->isLocallyOwned()) {
                // zero velocities by first deactivating the RigidBody
                btRigidBody* body = motionState->getRigidBody();
                if (body) {
                    body->forceActivationState(ISLAND_SLEEPING);
                }

                // send packet to remove ownership
                // NOTE: this packet will NOT be resent if lost, but the good news is:
                // the entity-server will eventually clear velocity and ownership for timeout
                motionState->sendUpdate(_entityPacketSender, _physicsEngine->getNumSubsteps());
            }

            // remove from the physical simulation
            _incomingChanges.remove(motionState);
            removeOwnershipData(motionState);
            _entitiesToRemoveFromPhysics.insert(entity);
            if (canBeKinematic && entity->isMovingRelativeToParent()) {
                SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
                if (itr == _simpleKinematicEntities.end()) {
                    _simpleKinematicEntities.insert(entity);
                }
            }
        } else {
            _incomingChanges.insert(motionState);
        }
        motionState->setRegion(region);
    } else if (shouldBePhysical) {
        // The intent is for this object to be in the PhysicsEngine, but it has no MotionState yet.
        // Perhaps it's shape has changed and it can now be added?
        _entitiesToAddToPhysics.insert(entity);
        SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
        if (itr != _simpleKinematicEntities.end()) {
            _simpleKinematicEntities.erase(itr);
        }
    } else if (canBeKinematic && entity->isMovingRelativeToParent()) {
        SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
        if (itr == _simpleKinematicEntities.end()) {
            _simpleKinematicEntities.insert(entity);
        }
    } else {
        SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
        if (itr != _simpleKinematicEntities.end()) {
            _simpleKinematicEntities.erase(itr);
        }
    }
}

void PhysicalEntitySimulation::processDeadEntities() {
    // Note: this override is a complete rewite of the base class's method because we cannot assume all entities
    // are domain entities, and the consequence of trying to delete a domain-entity in this case is very different.
    if (_deadEntitiesToRemoveFromTree.empty()) {
        return;
    }
    PROFILE_RANGE(simulation_physics, "Deletes");
    std::vector<EntityItemPointer> entitiesToDeleteImmediately;
    entitiesToDeleteImmediately.reserve(_deadEntitiesToRemoveFromTree.size());
    QUuid sessionID = Physics::getSessionUUID();
    QMutexLocker lock(&_mutex);
    for (auto entity : _deadEntitiesToRemoveFromTree) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _entitiesToRemoveFromPhysics.insert(entity);
        }
        if (entity->isDomainEntity()) {
            // interface-client can't delete domainEntities outright, they must roundtrip through the entity-server
            _entityPacketSender->queueEraseEntityMessage(entity->getID());
        } else if (entity->isLocalEntity() || entity->isMyAvatarEntity()) {
            entitiesToDeleteImmediately.push_back(entity);
            entity->collectChildrenForDelete(entitiesToDeleteImmediately, sessionID);
        }
    }
    _deadEntitiesToRemoveFromTree.clear();

    if (!entitiesToDeleteImmediately.empty()) {
        getEntityTree()->deleteEntitiesByPointer(entitiesToDeleteImmediately);
    }
}

void PhysicalEntitySimulation::clearEntities() {
    // TODO: we should probably wait to lock the _physicsEngine so we don't mess up data structures
    // while it is in the middle of a simulation step.  As it is, we're probably in shutdown mode
    // anyway, so maybe the simulation was already properly shutdown?  Cross our fingers...

    QMutexLocker lock(&_mutex);
    // remove the objects (aka MotionStates) from physics
    _physicsEngine->removeSetOfObjects(_physicalObjects);

    clearOwnershipData();

    // delete the MotionStates
    for (auto stateItr : _physicalObjects) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(&(*stateItr));
        assert(motionState);
        EntityItemPointer entity = motionState->getEntity();
        // TODO: someday when we invert the entities/physics lib dependencies we can let EntityItem delete its own PhysicsInfo
        // until then we must do it here
        delete motionState;
    }
    _physicalObjects.clear();

    // clear all other lists specific to this derived class
    _entitiesToRemoveFromPhysics.clear();
    _entitiesToAddToPhysics.clear();
    _incomingChanges.clear();
    _entitiesToDeleteLater.clear();

    EntitySimulation::clearEntities();
}

void PhysicalEntitySimulation::queueEraseDomainEntity(const QUuid& id) const {
    if (_entityPacketSender) {
        _entityPacketSender->queueEraseEntityMessage(id);
    }
}

// virtual
void PhysicalEntitySimulation::prepareEntityForDelete(EntityItemPointer entity) {
    // DANGER! this can be called on any thread
    // do no dirty deeds here --> assemble list for later
    assert(entity);
    assert(entity->isDead());
    QMutexLocker lock(&_mutex);
    _entitiesToDeleteLater.push_back(entity);
}

void PhysicalEntitySimulation::removeDeadEntities() {
    // DANGER! only ever call this on the main thread
    QMutexLocker lock(&_mutex);
    for (auto& entity : _entitiesToDeleteLater) {
        entity->clearActions(getThisPointer());
        EntitySimulation::prepareEntityForDelete(entity);
    }
    _entitiesToDeleteLater.clear();
}
// end EntitySimulation overrides

void PhysicalEntitySimulation::buildMotionStatesForEntitiesThatNeedThem() {
    // this lambda for when we decide to actually build the motionState
    auto buildMotionState = [&](btCollisionShape* shape, EntityItemPointer entity) {
        EntityMotionState* motionState = new EntityMotionState(shape, entity);
        entity->setPhysicsInfo(static_cast<void*>(motionState));
        motionState->setRegion(_space->getRegion(entity->getSpaceIndex()));
        _physicalObjects.insert(motionState);
        _incomingChanges.insert(motionState);
    };

    uint32_t deliveryCount = ObjectMotionState::getShapeManager()->getWorkDeliveryCount();
    if (deliveryCount != _lastWorkDeliveryCount) {
        // new off-thread shapes have arrived --> find adds whose shapes have arrived
        _lastWorkDeliveryCount = deliveryCount;
        ShapeRequests::iterator requestItr = _shapeRequests.begin();
        while (requestItr != _shapeRequests.end()) {
            EntityItemPointer entity = requestItr->entity;
            EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
            if (!motionState) {
                // this is an ADD because motionState doesn't exist yet
                btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShapeByKey(requestItr->shapeHash));
                if (shape) {
                    // shape is ready at last!
                    // but the entity's physics desired physics status may have changed since last requested
                    if (!entity->shouldBePhysical()) {
                        requestItr = _shapeRequests.erase(requestItr);
                        continue;
                    }
                    // rebuild the ShapeInfo to verify hash because entity's desired shape may have changed
                    // TODO? is there a better way to do this?
                    ShapeInfo shapeInfo;
                    entity->computeShapeInfo(shapeInfo);

                    if (shapeInfo.getType() == SHAPE_TYPE_NONE) {
                        requestItr = _shapeRequests.erase(requestItr);
                    } else if (shapeInfo.getHash() != requestItr->shapeHash) {
                        // bummer, the hashes are different and we no longer want the shape we've received
                        ObjectMotionState::getShapeManager()->releaseShape(shape);
                        // try again
                        shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
                        if (shape) {
                            buildMotionState(shape, entity);
                            requestItr = _shapeRequests.erase(requestItr);
                        } else {
                            requestItr->shapeHash = shapeInfo.getHash();
                            ++requestItr;
                        }
                    } else {
                        buildMotionState(shape, entity);
                        requestItr = _shapeRequests.erase(requestItr);
                    }
                } else {
                    // shape not ready
                    ++requestItr;
                }
            } else {
                // this is a CHANGE because motionState already exists
                if (ObjectMotionState::getShapeManager()->hasShapeWithKey(requestItr->shapeHash)) {
                    entity->markDirtyFlags(Simulation::DIRTY_SHAPE);
                    _incomingChanges.insert(motionState);
                    requestItr = _shapeRequests.erase(requestItr);
                } else {
                    // shape not ready
                    ++requestItr;
                }
            }
        }
    }

    SetOfEntities::iterator entityItr = _entitiesToAddToPhysics.begin();
    while (entityItr != _entitiesToAddToPhysics.end()) {
        EntityItemPointer entity = (*entityItr);
        if (entity->isDead()) {
            prepareEntityForDelete(entity);
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
            continue;
        }
        if (entity->getPhysicsInfo()) {
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
            continue;
        }
        if (!entity->shouldBePhysical()) {
            // this entity should no longer be on _entitiesToAddToPhysics
            if (entity->isMovingRelativeToParent()) {
                SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
                if (itr == _simpleKinematicEntities.end()) {
                    _simpleKinematicEntities.insert(entity);
                }
            }
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
            continue;
        }

        uint8_t region = _space->getRegion(entity->getSpaceIndex());
        if (region == workload::Region::UNKNOWN) {
            // the workload hasn't categorized it yet --> skip for later
            ++entityItr;
            continue;
        }
        if (region > workload::Region::R2) {
            // not in physical zone --> remove from list
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
            continue;
        }

        if (entity->isReadyToComputeShape()) {
            ShapeRequest shapeRequest(entity);
            ShapeRequests::iterator  requestItr = _shapeRequests.find(shapeRequest);
            if (requestItr == _shapeRequests.end()) {
                // not waiting for a shape (yet)
                ShapeInfo shapeInfo;
                entity->computeShapeInfo(shapeInfo);
                uint32_t requestCount = ObjectMotionState::getShapeManager()->getWorkRequestCount();
                btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
                if (shape) {
                    buildMotionState(shape, entity);
                } else if (requestCount != ObjectMotionState::getShapeManager()->getWorkRequestCount()) {
                    // shape doesn't exist but a new worker has been spawned to build it --> add to shapeRequests and wait
                    shapeRequest.shapeHash = shapeInfo.getHash();
                    _shapeRequests.insert(shapeRequest);
                } else {
                    // failed to build shape --> will not be added
                }
            }
            entityItr = _entitiesToAddToPhysics.erase(entityItr);
        } else {
            // skip for later
            ++entityItr;
        }
    }
}

void PhysicalEntitySimulation::buildPhysicsTransaction(PhysicsEngine::Transaction& transaction) {
    QMutexLocker lock(&_mutex);
    // entities being removed
    for (auto entity : _entitiesToRemoveFromPhysics) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            transaction.objectsToRemove.push_back(motionState);
            _incomingChanges.remove(motionState);
        }
        if (_shapeRequests.size() > 0) {
            ShapeRequest shapeRequest(entity);
            ShapeRequests::iterator  requestItr = _shapeRequests.find(shapeRequest);
            if (requestItr != _shapeRequests.end()) {
                _shapeRequests.erase(requestItr);
            }
        }
    }
    _entitiesToRemoveFromPhysics.clear();

    // entities to add
    buildMotionStatesForEntitiesThatNeedThem();

    // motionStates with changed entities: delete, add, or change
    for (auto& object : _incomingChanges) {
        uint32_t unhandledFlags = object->getIncomingDirtyFlags();

        uint32_t handledFlags = EASY_DIRTY_PHYSICS_FLAGS;
        bool isInPhysicsSimulation = object->isInPhysicsSimulation();
        bool shouldBeInPhysicsSimulation = object->shouldBeInPhysicsSimulation();
        if (!shouldBeInPhysicsSimulation && isInPhysicsSimulation) {
            transaction.objectsToRemove.push_back(object);
            continue;
        }

        bool needsNewShape = object->needsNewShape() && object->_entity->isReadyToComputeShape();
        if (needsNewShape) {
            ShapeType shapeType = object->getShapeType();
            if (shapeType == SHAPE_TYPE_STATIC_MESH) {
                ShapeRequest shapeRequest(object->_entity);
                ShapeRequests::iterator requestItr = _shapeRequests.find(shapeRequest);
                if (requestItr == _shapeRequests.end()) {
                    ShapeInfo shapeInfo;
                    object->_entity->computeShapeInfo(shapeInfo);
                    uint32_t requestCount = ObjectMotionState::getShapeManager()->getWorkRequestCount();
                    btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
                    if (shape) {
                        object->setShape(shape);
                        handledFlags |= Simulation::DIRTY_SHAPE;
                        needsNewShape = false;
                    } else if (requestCount != ObjectMotionState::getShapeManager()->getWorkRequestCount()) {
                        // shape doesn't exist but a new worker has been spawned to build it --> add to shapeRequests and wait
                        shapeRequest.shapeHash = shapeInfo.getHash();
                        _shapeRequests.insert(shapeRequest);
                    } else {
                        // failed to build shape --> will not be added/updated
                        handledFlags |= Simulation::DIRTY_SHAPE;
                    }
                } else {
                    // continue waiting for shape request
                }
            } else {
                ShapeInfo shapeInfo;
                object->_entity->computeShapeInfo(shapeInfo);
                btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
                if (shape) {
                    object->setShape(shape);
                    handledFlags |= Simulation::DIRTY_SHAPE;
                    needsNewShape = false;
                } else {
                    // failed to build shape --> will not be added
                }
            }
        }
        if (!isInPhysicsSimulation) {
            if (needsNewShape) {
                // skip it
                continue;
            } else {
                transaction.objectsToAdd.push_back(object);
                handledFlags = DIRTY_PHYSICS_FLAGS;
                unhandledFlags = 0;
            }
        }

        if (unhandledFlags & EASY_DIRTY_PHYSICS_FLAGS) {
            object->handleEasyChanges(unhandledFlags);
        }
        if (unhandledFlags & (Simulation::DIRTY_MOTION_TYPE | Simulation::DIRTY_COLLISION_GROUP | (handledFlags & Simulation::DIRTY_SHAPE))) {
            transaction.objectsToReinsert.push_back(object);
            handledFlags |= HARD_DIRTY_PHYSICS_FLAGS;
        } else if (unhandledFlags & Simulation::DIRTY_PHYSICS_ACTIVATION && object->getRigidBody()->isStaticObject()) {
            transaction.activeStaticObjects.push_back(object);
        }
        object->clearIncomingDirtyFlags(handledFlags);
    }
    _incomingChanges.clear();
}

void PhysicalEntitySimulation::handleProcessedPhysicsTransaction(PhysicsEngine::Transaction& transaction) {
    // things on objectsToRemove are ready for delete
    for (auto object : transaction.objectsToRemove) {
        EntityMotionState* entityState = static_cast<EntityMotionState*>(object);
        removeOwnershipData(entityState);
        _physicalObjects.remove(object);
        delete object;
    }
    transaction.clear();
}

void PhysicalEntitySimulation::handleDeactivatedMotionStates(const VectorOfMotionStates& motionStates) {
    bool serverlessMode = getEntityTree()->isServerlessMode();
    for (auto stateItr : motionStates) {
        ObjectMotionState* state = &(*stateItr);
        assert(state);
        if (state->getType() == MOTIONSTATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            EntityItemPointer entity = entityState->getEntity();
            _entitiesToSort.insert(entity);
            if (!serverlessMode) {
                if (entity->isAvatarEntity()) {
                    switch (entityState->getOwnershipState()) {
                        case EntityMotionState::OwnershipState::PendingBid:
                            _bids.removeFirst(entityState);
                            entityState->clearOwnershipState();
                            break;
                        case EntityMotionState::OwnershipState::LocallyOwned:
                            _owned.removeFirst(entityState);
                            entityState->clearOwnershipState();
                            break;
                        default:
                            break;
                    }
                } else {
                    entityState->handleDeactivation();
                }
            }
        }
    }
}

void PhysicalEntitySimulation::handleChangedMotionStates(const VectorOfMotionStates& motionStates) {
    PROFILE_RANGE_EX(simulation_physics, "ChangedEntities", 0x00000000, (uint64_t)motionStates.size());
    QMutexLocker lock(&_mutex);

    for (auto stateItr : motionStates) {
        ObjectMotionState* state = &(*stateItr);
        assert(state);
        if (state->getType() == MOTIONSTATE_TYPE_ENTITY) {
            EntityMotionState* entityState = static_cast<EntityMotionState*>(state);
            _entitiesToSort.insert(entityState->getEntity());
            if (entityState->getOwnershipState() == EntityMotionState::OwnershipState::NotLocallyOwned) {
                // NOTE: entityState->getOwnershipState() reflects what ownership list (_bids or _owned) it is in
                // and is distinct from entityState->isLocallyOwned() which checks the simulation ownership
                // properties of the corresponding EntityItem.  It is possible for the two states to be out
                // of sync.  In fact, we're trying to put them back into sync here.
                if (entityState->isLocallyOwned()) {
                    addOwnership(entityState);
                } else if (entityState->shouldSendBid()) {
                    addOwnershipBid(entityState);
                } else {
                    entityState->getEntity()->updateQueryAACube();
                }
            }
        }
    }

    uint32_t numSubsteps = _physicsEngine->getNumSubsteps();
    if (_lastStepSendPackets != numSubsteps) {
        _lastStepSendPackets = numSubsteps;

        if (Physics::getSessionUUID().isNull()) {
            // usually don't get here, but if so clear all ownership
            clearOwnershipData();
        }
        // send updates before bids, because this simplifies the logic thasuccessful bids will immediately send an update when added to the 'owned' list
        sendOwnedUpdates(numSubsteps);
        sendOwnershipBids(numSubsteps);
    }
}

void PhysicalEntitySimulation::addOwnershipBid(EntityMotionState* motionState) {
    if (getEntityTree()->isServerlessMode()) {
        return;
    }
    motionState->initForBid();
    motionState->sendBid(_entityPacketSender, _physicsEngine->getNumSubsteps());
    _bids.push_back(motionState);
    _nextBidExpiry = glm::min(_nextBidExpiry, motionState->getNextBidExpiry());
}

void PhysicalEntitySimulation::addOwnership(EntityMotionState* motionState) {
    if (getEntityTree()->isServerlessMode()) {
        return;
    }
    motionState->initForOwned();
    _owned.push_back(motionState);
}

void PhysicalEntitySimulation::sendOwnershipBids(uint32_t numSubsteps) {
    uint64_t now = usecTimestampNow();
    if (now > _nextBidExpiry) {
        PROFILE_RANGE_EX(simulation_physics, "Bid", 0x00000000, (uint64_t)_bids.size());
        _nextBidExpiry = std::numeric_limits<uint64_t>::max();
        uint32_t i = 0;
        while (i < _bids.size()) {
            bool removeBid = false;
            if (_bids[i]->isLocallyOwned()) {
                // when an object transitions from 'bid' to 'owned' we are changing the "mode" of data stored
                // in the EntityMotionState::_serverFoo variables (please see comments in EntityMotionState.h)
                // therefore we need to immediately send an update so that the values stored are what we're
                // "telling" the server rather than what we've been "hearing" from the server.
                _bids[i]->slaveBidPriority();
                _bids[i]->sendUpdate(_entityPacketSender, numSubsteps);

                addOwnership(_bids[i]);
                removeBid = true;
            } else if (!_bids[i]->shouldSendBid()) {
                removeBid = true;
                _bids[i]->clearOwnershipState();
            }
            if (removeBid) {
                _bids.remove(i);
            } else {
                if (now > _bids[i]->getNextBidExpiry()) {
                    _bids[i]->sendBid(_entityPacketSender, numSubsteps);
                    _nextBidExpiry = glm::min(_nextBidExpiry, _bids[i]->getNextBidExpiry());
                }
                ++i;
            }
        }
    }
}

void PhysicalEntitySimulation::sendOwnedUpdates(uint32_t numSubsteps) {
    if (getEntityTree()->isServerlessMode()) {
        return;
    }
    PROFILE_RANGE_EX(simulation_physics, "Update", 0x00000000, (uint64_t)_owned.size());
    uint32_t i = 0;
    while (i < _owned.size()) {
        if (!_owned[i]->isLocallyOwned()) {
            if (_owned[i]->shouldSendBid()) {
                addOwnershipBid(_owned[i]);
            } else {
                _owned[i]->clearOwnershipState();
            }
            _owned.remove(i);
        } else {
            if (_owned[i]->shouldSendUpdate(numSubsteps)) {
                _owned[i]->sendUpdate(_entityPacketSender, numSubsteps);
            }
            ++i;
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
        QMutexLocker lock(&_dynamicsMutex);
        _dynamicsToAdd += dynamic;
    }
}

void PhysicalEntitySimulation::removeDynamic(const QUuid dynamicID) {
    QMutexLocker lock(&_dynamicsMutex);
    _dynamicsToRemove += dynamicID;
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
        _dynamicsToAdd.clear();
        _dynamicsToRemove.clear();
    }

    // put back the ones that couldn't yet be added
    foreach (EntityDynamicPointer dynamicFailedToAdd, dynamicsFailedToAdd) {
        addDynamic(dynamicFailedToAdd);
    }
}
