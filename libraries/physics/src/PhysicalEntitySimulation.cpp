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
void PhysicalEntitySimulation::updateEntitiesInternal(uint64_t now) {
    // Do nothing here because the "internal" update the PhysicsEngine::stepSimulation() which is done elsewhere.
}

void PhysicalEntitySimulation::addEntityInternal(EntityItemPointer entity) {
    QMutexLocker lock(&_mutex);
    assert(entity);
    assert(!entity->isDead());
    uint8_t region = _space->getRegion(entity->getSpaceIndex());
    bool shouldBePhysical = region < workload::Region::R3 && entity->shouldBePhysical();
    bool canBeKinematic = region <= workload::Region::R3;
    if (shouldBePhysical) {
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

void PhysicalEntitySimulation::removeEntityInternal(EntityItemPointer entity) {
    if (entity->isSimulated()) {
        EntitySimulation::removeEntityInternal(entity);
        _entitiesToAddToPhysics.remove(entity);

        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            removeOwnershipData(motionState);
            _entitiesToRemoveFromPhysics.insert(entity);
        } else if (entity->isDead() && entity->getElement()) {
            _deadEntities.insert(entity);
        }
    }
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

void PhysicalEntitySimulation::takeDeadEntities(SetOfEntities& deadEntities) {
    QMutexLocker lock(&_mutex);
    for (auto entity : _deadEntities) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        if (motionState) {
            _entitiesToRemoveFromPhysics.insert(entity);
        }
    }
    _deadEntities.swap(deadEntities);
    _deadEntities.clear();
}

void PhysicalEntitySimulation::changeEntityInternal(EntityItemPointer entity) {
    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    QMutexLocker lock(&_mutex);
    assert(entity);
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
            _physicalObjects.remove(motionState);
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

void PhysicalEntitySimulation::clearEntitiesInternal() {
    // TODO: we should probably wait to lock the _physicsEngine so we don't mess up data structures
    // while it is in the middle of a simulation step.  As it is, we're probably in shutdown mode
    // anyway, so maybe the simulation was already properly shutdown?  Cross our fingers...

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
}

// virtual
void PhysicalEntitySimulation::prepareEntityForDelete(EntityItemPointer entity) {
    assert(entity);
    assert(entity->isDead());
    QMutexLocker lock(&_mutex);
    entity->clearActions(getThisPointer());
    removeEntityInternal(entity);
}
// end EntitySimulation overrides

const VectorOfMotionStates& PhysicalEntitySimulation::getObjectsToRemoveFromPhysics() {
    QMutexLocker lock(&_mutex);
    for (auto entity: _entitiesToRemoveFromPhysics) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(entity->getPhysicsInfo());
        assert(motionState);
      // TODO CLEan this, just a n extra check to avoid the crash that shouldn;t happen
      if (motionState) {
            _entitiesToAddToPhysics.remove(entity);
            if (entity->isDead() && entity->getElement()) {
                _deadEntities.insert(entity);
            }

            _incomingChanges.remove(motionState);
            removeOwnershipData(motionState);
            _physicalObjects.remove(motionState);

            // remember this motionState and delete it later (after removing its RigidBody from the PhysicsEngine)
            _objectsToDelete.push_back(motionState);
        }
    }
    _entitiesToRemoveFromPhysics.clear();
    return _objectsToDelete;
}

void PhysicalEntitySimulation::deleteObjectsRemovedFromPhysics() {
    QMutexLocker lock(&_mutex);
    for (auto motionState : _objectsToDelete) {
        // someday when we invert the entities/physics lib dependencies we can let EntityItem delete its own PhysicsInfo
        // until then we must do it here
        // NOTE: a reference to the EntityItemPointer is released in the EntityMotionState::dtor
        delete motionState;
    }
    _objectsToDelete.clear();
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
                SetOfEntities::iterator itr = _simpleKinematicEntities.find(entity);
                if (itr == _simpleKinematicEntities.end()) {
                    _simpleKinematicEntities.insert(entity);
                }
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

                // make sure the motionState's region is up-to-date before it is actually added to physics
                motionState->setRegion(_space->getRegion(entity->getSpaceIndex()));
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
        _incomingChanges.insert(static_cast<EntityMotionState*>(object));
    }
}

void PhysicalEntitySimulation::getObjectsToChange(VectorOfMotionStates& result) {
    result.clear();
    QMutexLocker lock(&_mutex);
    for (auto stateItr : _incomingChanges) {
        EntityMotionState* motionState = &(*stateItr);
        result.push_back(motionState);
    }
    _incomingChanges.clear();
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
                if (entity->getClientOnly()) {
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
