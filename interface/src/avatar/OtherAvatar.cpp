//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OtherAvatar.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <AvatarLogging.h>

#include "Application.h"
#include "AvatarMotionState.h"

const float DISPLAYNAME_FADE_TIME = 0.5f;
const float DISPLAYNAME_FADE_FACTOR = pow(0.01f, 1.0f / DISPLAYNAME_FADE_TIME);

static glm::u8vec3 getLoadingOrbColor(Avatar::LoadingStatus loadingStatus) {

    const glm::u8vec3 NO_MODEL_COLOR(0xe3, 0xe3, 0xe3);
    const glm::u8vec3 LOAD_MODEL_COLOR(0xef, 0x93, 0xd1);
    const glm::u8vec3 LOAD_SUCCESS_COLOR(0x1f, 0xc6, 0xa6);
    const glm::u8vec3 LOAD_FAILURE_COLOR(0xc6, 0x21, 0x47);
    switch (loadingStatus) {
    case Avatar::LoadingStatus::NoModel:
        return NO_MODEL_COLOR;
    case Avatar::LoadingStatus::LoadModel:
        return LOAD_MODEL_COLOR;
    case Avatar::LoadingStatus::LoadSuccess:
        return LOAD_SUCCESS_COLOR;
    case Avatar::LoadingStatus::LoadFailure:
    default:
        return LOAD_FAILURE_COLOR;
    }
}

OtherAvatar::OtherAvatar(QThread* thread) : Avatar(thread) {
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = new Head(this);
    _skeletonModel = std::make_shared<SkeletonModel>(this, nullptr);
    _skeletonModel->setLoadingPriority(OTHERAVATAR_LOADING_PRIORITY);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, &Avatar::setModelURLFinished);
    connect(_skeletonModel.get(), &Model::rigReady, this, &Avatar::rigReady);
    connect(_skeletonModel.get(), &Model::rigReset, this, &Avatar::rigReset);
}

OtherAvatar::~OtherAvatar() {
    removeOrb();
}

void OtherAvatar::removeOrb() {
    if (!_otherAvatarOrbMeshPlaceholderID.isNull()) {
        DependencyManager::get<EntityScriptingInterface>()->deleteEntity(_otherAvatarOrbMeshPlaceholderID);
        _otherAvatarOrbMeshPlaceholderID = UNKNOWN_ENTITY_ID;
    }
}

void OtherAvatar::updateOrbPosition() {
    if (_otherAvatarOrbMeshPlaceholderID.isNull()) {
        EntityItemProperties properties;
        properties.setPosition(getHead()->getPosition());
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_otherAvatarOrbMeshPlaceholderID, properties);
    }
}

void OtherAvatar::createOrb() {
    if (_otherAvatarOrbMeshPlaceholderID.isNull()) {
        EntityItemProperties properties;
        properties.setType(EntityTypes::Sphere);
        properties.setAlpha(1.0f);
        properties.setColor(getLoadingOrbColor(_loadingStatus));
        properties.setPrimitiveMode(PrimitiveMode::LINES);
        properties.getPulse().setMin(0.5f);
        properties.getPulse().setMax(1.0f);
        properties.getPulse().setColorMode(PulseMode::IN_PHASE);
        properties.setIgnorePickIntersection(true);

        properties.setPosition(getHead()->getPosition());
        properties.setRotation(glm::quat(0.0f, 0.0f, 0.0f, 1.0));
        properties.setDimensions(glm::vec3(0.5f, 0.5f, 0.5f));
        properties.setVisible(true);

        _otherAvatarOrbMeshPlaceholderID = DependencyManager::get<EntityScriptingInterface>()->addEntityInternal(properties, entity::HostType::LOCAL);
    }
}

void OtherAvatar::indicateLoadingStatus(LoadingStatus loadingStatus) {
    Avatar::indicateLoadingStatus(loadingStatus);

    if (_otherAvatarOrbMeshPlaceholderID != UNKNOWN_ENTITY_ID) {
        EntityItemProperties properties;
        properties.setColor(getLoadingOrbColor(_loadingStatus));
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_otherAvatarOrbMeshPlaceholderID, properties);
    }
}

void OtherAvatar::setSpaceIndex(int32_t index) {
    assert(_spaceIndex == -1);
    _spaceIndex = index;
}

void OtherAvatar::updateSpaceProxy(workload::Transaction& transaction) const {
    if (_spaceIndex > -1) {
        float approximateBoundingRadius = glm::length(getTargetScale());
        workload::Sphere sphere(getWorldPosition(), approximateBoundingRadius);
        transaction.update(_spaceIndex, sphere);
    }
}

int OtherAvatar::parseDataFromBuffer(const QByteArray& buffer) {
    int32_t bytesRead = Avatar::parseDataFromBuffer(buffer);
    if (_moving && _motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_POSITION);
    }
    return bytesRead;
}

void OtherAvatar::setWorkloadRegion(uint8_t region) {
    _workloadRegion = region;
}

bool OtherAvatar::shouldBeInPhysicsSimulation() const {
    return (_workloadRegion < workload::Region::R3 && !isDead());
}

bool OtherAvatar::needsPhysicsUpdate() const {
    constexpr uint32_t FLAGS_OF_INTEREST = Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS | Simulation::DIRTY_POSITION | Simulation::DIRTY_COLLISION_GROUP;
    return (_motionState && (bool)(_motionState->getIncomingDirtyFlags() & FLAGS_OF_INTEREST));
}

void OtherAvatar::rebuildCollisionShape() {
    if (_motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
    }
}

void OtherAvatar::updateCollisionGroup(bool myAvatarCollide) {
    if (_motionState) {
        bool collides = _motionState->getCollisionGroup() == BULLET_COLLISION_GROUP_OTHER_AVATAR && myAvatarCollide;
        if (_collideWithOtherAvatars != collides) {
            if (!myAvatarCollide) {
                _collideWithOtherAvatars = false;
            }
            auto newCollisionGroup = _collideWithOtherAvatars ? BULLET_COLLISION_GROUP_OTHER_AVATAR : BULLET_COLLISION_GROUP_COLLISIONLESS;
            _motionState->setCollisionGroup(newCollisionGroup);
            _motionState->addDirtyFlags(Simulation::DIRTY_COLLISION_GROUP);
        }
    }
}

void OtherAvatar::simulate(float deltaTime, bool inView) {
    PROFILE_RANGE(simulation, "simulate");

    _globalPosition = _transit.isActive() ? _transit.getCurrentPosition() : _serverPosition;
    if (!hasParent()) {
        setLocalPosition(_globalPosition);
    }

    _simulationRate.increment();
    if (inView) {
        _simulationInViewRate.increment();
    }

    PerformanceTimer perfTimer("simulate");
    {
        PROFILE_RANGE(simulation, "updateJoints");
        if (inView) {
            Head* head = getHead();
            if (_hasNewJointData || _transit.isActive()) {
                _skeletonModel->getRig().copyJointsFromJointData(_jointData);
                glm::mat4 rootTransform = glm::scale(_skeletonModel->getScale()) * glm::translate(_skeletonModel->getOffset());
                _skeletonModel->getRig().computeExternalPoses(rootTransform);
                _jointDataSimulationRate.increment();

                _skeletonModel->simulate(deltaTime, true);

                locationChanged(); // joints changed, so if there are any children, update them.
                _hasNewJointData = false;

                glm::vec3 headPosition = getWorldPosition();
                if (!_skeletonModel->getHeadPosition(headPosition)) {
                    headPosition = getWorldPosition();
                }
                head->setPosition(headPosition);
            }
            head->setScale(getModelScale());
            head->simulate(deltaTime);
            relayJointDataToChildren();
        } else {
            // a non-full update is still required so that the position, rotation, scale and bounds of the skeletonModel are updated.
            _skeletonModel->simulate(deltaTime, false);
        }
        _skeletonModelSimulationRate.increment();
    }

    // update animation for display name fade in/out
    if ( _displayNameTargetAlpha != _displayNameAlpha) {
        // the alpha function is
        // Fade out => alpha(t) = factor ^ t => alpha(t+dt) = alpha(t) * factor^(dt)
        // Fade in  => alpha(t) = 1 - factor^t => alpha(t+dt) = 1-(1-alpha(t))*coef^(dt)
        // factor^(dt) = coef
        float coef = pow(DISPLAYNAME_FADE_FACTOR, deltaTime);
        if (_displayNameTargetAlpha < _displayNameAlpha) {
            // Fading out
            _displayNameAlpha *= coef;
        } else {
            // Fading in
            _displayNameAlpha = 1.0f - (1.0f - _displayNameAlpha) * coef;
        }
        _displayNameAlpha = glm::abs(_displayNameAlpha - _displayNameTargetAlpha) < 0.01f ? _displayNameTargetAlpha : _displayNameAlpha;
    }

    {
        PROFILE_RANGE(simulation, "misc");
        measureMotionDerivatives(deltaTime);
        simulateAttachments(deltaTime);
        updatePalms();
    }
    {
        PROFILE_RANGE(simulation, "entities");
        handleChangedAvatarEntityData();
        updateAttachedAvatarEntities();
    }

    {
        PROFILE_RANGE(simulation, "grabs");
        updateGrabs();
    }

    updateFadingStatus();
}

void OtherAvatar::handleChangedAvatarEntityData() {
    PerformanceTimer perfTimer("attachments");

    // AVATAR ENTITY UPDATE FLOW
    // - if queueEditEntityMessage() sees "AvatarEntity" HostType it calls _myAvatar->storeAvatarEntityDataPayload()
    // - storeAvatarEntityDataPayload() saves the payload and flags the trait instance for the entity as updated,
    // - ClientTraitsHandler::sendChangedTraitsToMixea() sends the entity bytes to the mixer which relays them to other interfaces
    // - AvatarHashMap::processBulkAvatarTraits() on other interfaces calls avatar->processTraitInstance()
    // - AvatarData::processTraitInstance() calls storeAvatarEntityDataPayload(), which sets _avatarEntityDataChanged = true
    // - (My)Avatar::simulate() calls handleChangedAvatarEntityData() every frame which checks _avatarEntityDataChanged
    // and here we are...

    // AVATAR ENTITY DELETE FLOW
    // - EntityScriptingInterface::deleteEntity() calls _myAvatar->clearAvatarEntity() for deleted avatar entities
    // - clearAvatarEntity() removes the avatar entity and flags the trait instance for the entity as deleted
    // - ClientTraitsHandler::sendChangedTraitsToMixer() sends a deletion to the mixer which relays to other interfaces
    // - AvatarHashMap::processBulkAvatarTraits() on other interfaces calls avatar->processDeletedTraitInstace()
    // - AvatarData::processDeletedTraitInstance() calls clearAvatarEntity()
    // - AvatarData::clearAvatarEntity() sets _avatarEntityDataChanged = true and adds the ID to the detached list
    // - (My)Avatar::simulate() calls handleChangedAvatarEntityData() every frame which checks _avatarEntityDataChanged
    // and here we are...

    if (!_avatarEntityDataChanged) {
        return;
    }

    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (!entityTree) {
        return;
    }

    PackedAvatarEntityMap packedAvatarEntityData;
    _avatarEntitiesLock.withReadLock([&] {
        packedAvatarEntityData = _packedAvatarEntityData;
    });
    entityTree->withWriteLock([&] {
        AvatarEntityMap::const_iterator dataItr = packedAvatarEntityData.begin();
        while (dataItr != packedAvatarEntityData.end()) {
            // compute hash of data.  TODO? cache this?
            QByteArray data = dataItr.value();
            uint32_t newHash = qHash(data);

            // check to see if we recognize this hash and whether it was already successfully processed
            QUuid entityID = dataItr.key();
            MapOfAvatarEntityDataHashes::iterator stateItr = _avatarEntityDataHashes.find(entityID);
            if (stateItr != _avatarEntityDataHashes.end()) {
                if (stateItr.value().success) {
                    if (newHash == stateItr.value().hash) {
                        // data hasn't changed --> nothing to do
                        ++dataItr;
                        continue;
                    }
                } else {
                    // NOTE: if the data was unsuccessful in producing an entity in the past
                    // we will try again just in case something changed (unlikely).
                    // Unfortunately constantly trying to build the entity for this data costs
                    // CPU cycles that we'd rather not spend.
                    // TODO? put a maximum number of tries on this?
                }
            } else {
                // sanity check data
                QUuid id;
                EntityTypes::EntityType type;
                EntityTypes::extractEntityTypeAndID((unsigned char*)(data.data()), data.size(), type, id);
                if (id != entityID || !EntityTypes::typeIsValid(type)) {
                    // skip for corrupt
                    ++dataItr;
                    continue;
                }
                // remember this hash for the future
                stateItr = _avatarEntityDataHashes.insert(entityID, AvatarEntityDataHash(newHash));
            }
            ++dataItr;

            EntityItemProperties properties;
            int32_t bytesLeftToRead = data.size();
            unsigned char* dataAt = (unsigned char*)(data.data());
            if (!properties.constructFromBuffer(dataAt, bytesLeftToRead)) {
                // properties are corrupt
                continue;
            }

            properties.setEntityHostType(entity::HostType::AVATAR);
            properties.setOwningAvatarID(getID());

            // there's no entity-server to tell us we're the simulation owner, so always set the
            // simulationOwner to the owningAvatarID and a high priority.
            properties.setSimulationOwner(getID(), AVATAR_ENTITY_SIMULATION_PRIORITY);

            if (properties.getParentID() == AVATAR_SELF_ID) {
                properties.setParentID(getID());
            }

            // NOTE: if this avatar entity is not attached to us, strip its entity script completely...
            auto attachedScript = properties.getScript();
            if (!isMyAvatar() && !attachedScript.isEmpty()) {
                QString noScript;
                properties.setScript(noScript);
            }

            auto specifiedHref = properties.getHref();
            if (!isMyAvatar() && !specifiedHref.isEmpty()) {
                qCDebug(avatars) << "removing entity href from avatar attached entity:" << entityID << "old href:" << specifiedHref;
                QString noHref;
                properties.setHref(noHref);
            }

            // When grabbing avatar entities, they are parented to the joint moving them, then when un-grabbed
            // they go back to the default parent (null uuid).  When un-gripped, others saw the entity disappear.
            // The thinking here is the local position was noticed as changing, but not the parentID (since it is now
            // back to the default), and the entity flew off somewhere.  Marking all changed definitely fixes this,
            // and seems safe (per Seth).
            properties.markAllChanged();

            // try to build the entity
            EntityItemPointer entity = entityTree->findEntityByEntityItemID(EntityItemID(entityID));
            bool success = true;
            if (entity) {
                QUuid oldParentID = entity->getParentID();
                if (entityTree->updateEntity(entityID, properties)) {
                    entity->updateLastEditedFromRemote();
                } else {
                    success = false;
                }
                if (oldParentID != entity->getParentID()) {
                    if (entity->getParentID() == getID()) {
                        onAddAttachedAvatarEntity(entityID);
                    } else if (oldParentID == getID()) {
                        onRemoveAttachedAvatarEntity(entityID);
                    }
                }
            } else {
                entity = entityTree->addEntity(entityID, properties);
                if (!entity) {
                    success = false;
                } else if (entity->getParentID() == getID()) {
                    onAddAttachedAvatarEntity(entityID);
                }
            }
            stateItr.value().success = success;
        }

        AvatarEntityIDs recentlyRemovedAvatarEntities = getAndClearRecentlyRemovedIDs();
        if (!recentlyRemovedAvatarEntities.empty()) {
            // only lock this thread when absolutely necessary
            AvatarEntityMap packedAvatarEntityData;
            _avatarEntitiesLock.withReadLock([&] {
                packedAvatarEntityData = _packedAvatarEntityData;
            });
            foreach (auto entityID, recentlyRemovedAvatarEntities) {
                if (!packedAvatarEntityData.contains(entityID)) {
                    entityTree->deleteEntity(entityID, true, true);
                }
            }

            // TODO: move this outside of tree lock
            // remove stale data hashes
            foreach (auto entityID, recentlyRemovedAvatarEntities) {
                MapOfAvatarEntityDataHashes::iterator stateItr = _avatarEntityDataHashes.find(entityID);
                if (stateItr != _avatarEntityDataHashes.end()) {
                    _avatarEntityDataHashes.erase(stateItr);
                }
                onRemoveAttachedAvatarEntity(entityID);
            }
        }
        if (packedAvatarEntityData.size() != _avatarEntityForRecording.size()) {
            createRecordingIDs();
        }
    });

    setAvatarEntityDataChanged(false);
}

void OtherAvatar::onAddAttachedAvatarEntity(const QUuid& id) {
    for (uint32_t i = 0; i < _attachedAvatarEntities.size(); ++i) {
        if (_attachedAvatarEntities[i] == id) {
            return;
        }
    }
    _attachedAvatarEntities.push_back(id);
}

void OtherAvatar::onRemoveAttachedAvatarEntity(const QUuid& id) {
    for (uint32_t i = 0; i < _attachedAvatarEntities.size(); ++i) {
        if (_attachedAvatarEntities[i] == id) {
            if (i != _attachedAvatarEntities.size() - 1) {
                _attachedAvatarEntities[i] = _attachedAvatarEntities.back();
            }
            _attachedAvatarEntities.pop_back();
            break;
        }
    }
}

void OtherAvatar::updateAttachedAvatarEntities() {
    if (!_attachedAvatarEntities.empty()) {
        auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
        if (!treeRenderer) {
            return;
        }
        for (const QUuid& id : _attachedAvatarEntities) {
            treeRenderer->onEntityChanged(id);
        }
    }
}
