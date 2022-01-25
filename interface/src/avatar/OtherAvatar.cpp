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
#include "DetailedMotionState.h"
#include "DebugDraw.h"

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
    if (!_otherAvatarOrbMeshPlaceholderID.isNull()) {
        EntityItemProperties properties;
        glm::vec3 headPosition;
        if (!_skeletonModel->getHeadPosition(headPosition)) {
            headPosition = getWorldPosition();
        }
        properties.setPosition(headPosition);
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_otherAvatarOrbMeshPlaceholderID, properties);
    }
}

void OtherAvatar::createOrb() {
    if (_otherAvatarOrbMeshPlaceholderID.isNull()) {
        EntityItemProperties properties;
        properties.setType(EntityTypes::Sphere);
        properties.setAlpha(1.0f);
        properties.setColor(getLoadingOrbColor(_loadingStatus));
        properties.setName("Loading Avatar " + getID().toString());
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
    for (auto& detailedMotionState : _detailedMotionStates) {
        // NOTE: we activate _detailedMotionStates is because they are KINEMATIC
        // and Bullet will automagically call DetailedMotionState::getWorldTransform() when active.
        detailedMotionState->forceActive();
    }
    if (_moving && _motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_POSITION);
    }
    return bytesRead;
}

const btCollisionShape* OtherAvatar::createCollisionShape(int32_t jointIndex, bool& isBound, std::vector<int32_t>& boundJoints) {
    ShapeInfo shapeInfo;
    isBound = false;
    QString jointName = "";
    if (jointIndex > -1 && jointIndex < (int32_t)_multiSphereShapes.size()) {
        jointName = _multiSphereShapes[jointIndex].getJointName();
    }
    switch (_bodyLOD) {
    case BodyLOD::Sphere:
        shapeInfo.setSphere(0.5f * getFitBounds().getDimensions().y);
        boundJoints.clear();
        for (auto &spheres : _multiSphereShapes) {
            if (spheres.isValid()) {
                boundJoints.push_back(spheres.getJointIndex());
            }
        }
        isBound = true;
        break;
    case BodyLOD::MultiSphereLow:
        if (jointName.contains("RightHand", Qt::CaseInsensitive) || jointName.contains("LeftHand", Qt::CaseInsensitive))  {
            if (jointName.size() <= QString("RightHand").size()) {
                AABox handBound;
                for (auto &spheres : _multiSphereShapes) {
                    if (spheres.isValid() && spheres.getJointName().contains(jointName, Qt::CaseInsensitive)) {
                        boundJoints.push_back(spheres.getJointIndex());
                        handBound += spheres.getBoundingBox();
                    }
                }
                shapeInfo.setSphere(0.5f * handBound.getLargestDimension());
                glm::vec3 jointPosition;
                glm::quat jointRotation;
                _skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPosition);
                _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRotation);
                glm::vec3 positionOffset = glm::inverse(jointRotation) * (handBound.calcCenter() - jointPosition);
                shapeInfo.setOffset(positionOffset);
                isBound = true;
            }
            break;
        }
        // Note: MultiSphereLow case really means: "skip fingers and use spheres for hands,
        // else fall through to MultiSphereHigh case"
        /* fall-thru */
    case BodyLOD::MultiSphereHigh:
        computeDetailedShapeInfo(shapeInfo, jointIndex);
        break;
    default:
        assert(false); // should never reach here
        break;
    }
    return ObjectMotionState::getShapeManager()->getShape(shapeInfo);
}

void OtherAvatar::forgetDetailedMotionStates() {
    // NOTE: the DetailedMotionStates are deleted after being added to PhysicsEngine::Transaction::_objectsToRemove
    // See AvatarManager::handleProcessedPhysicsTransaction()
    _detailedMotionStates.clear();
}

void OtherAvatar::setWorkloadRegion(uint8_t region) {
    _workloadRegion = region;
    computeShapeLOD();
}

void OtherAvatar::computeShapeLOD() {
    // auto newBodyLOD = _workloadRegion < workload::Region::R3 ? BodyLOD::MultiSphereShapes : BodyLOD::CapsuleShape;
    // auto newBodyLOD = BodyLOD::CapsuleShape;
    BodyLOD newLOD;
    switch (_workloadRegion) {
    case workload::Region::R1:
        newLOD = BodyLOD::MultiSphereHigh;
        break;
    case workload::Region::R2:
        newLOD = BodyLOD::MultiSphereLow;
        break;
    case workload::Region::UNKNOWN:
    case workload::Region::INVALID:
    case workload::Region::R4:
    case workload::Region::R3:
    default:
        newLOD = BodyLOD::Sphere;
        break;
    }
    if (newLOD != _bodyLOD) {
        _bodyLOD = newLOD;
        if (isInPhysicsSimulation()) {
            _needsDetailedRebuild = true;
        }
    }
}

bool OtherAvatar::isInPhysicsSimulation() const {
    return _motionState && _motionState->getRigidBody();
}

bool OtherAvatar::shouldBeInPhysicsSimulation() const {
    return !isDead() && _workloadRegion <= workload::Region::R3;
}

bool OtherAvatar::needsPhysicsUpdate() const {
    constexpr uint32_t FLAGS_OF_INTEREST = Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS | Simulation::DIRTY_POSITION | Simulation::DIRTY_COLLISION_GROUP;
    return (_needsDetailedRebuild || (_motionState && (bool)(_motionState->getIncomingDirtyFlags() & FLAGS_OF_INTEREST)));
}

void OtherAvatar::rebuildCollisionShape() {
    if (_motionState) {
        // do not actually rebuild here, instead flag for later
        _motionState->addDirtyFlags(Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
        _needsDetailedRebuild = true;
    }
}

void OtherAvatar::setCollisionWithOtherAvatarsFlags() {
    if (_motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_COLLISION_GROUP);
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

                head->simulate(deltaTime);
                _skeletonModel->simulate(deltaTime, true);

                locationChanged(); // joints changed, so if there are any children, update them.
                _hasNewJointData = false;

                glm::vec3 headPosition = getWorldPosition();
                if (!_skeletonModel->getHeadPosition(headPosition)) {
                    headPosition = getWorldPosition();
                }
                head->setPosition(headPosition);
            } else {
                head->simulate(deltaTime);
                _skeletonModel->simulate(deltaTime, false);
            }
            head->setScale(getModelScale());
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
        applyGrabChanges();
    }
}

void OtherAvatar::debugJointData() const {
    // Get a copy of the joint data
    auto jointData = getJointData();
    auto skeletonData = getSkeletonData();
    if ((int)skeletonData.size() == jointData.size() && jointData.size() != 0) {
        const vec4 RED(1.0f, 0.0f, 0.0f, 1.0f);
        const vec4 GREEN(0.0f, 1.0f, 0.0f, 1.0f);
        const vec4 BLUE(0.0f, 0.0f, 1.0f, 1.0f);
        const vec4 LIGHT_RED(1.0f, 0.5f, 0.5f, 1.0f);
        const vec4 LIGHT_GREEN(0.5f, 1.0f, 0.5f, 1.0f);
        const vec4 LIGHT_BLUE(0.5f, 0.5f, 1.0f, 1.0f);
        const vec4 GREY(0.3f, 0.3f, 0.3f, 1.0f);
        const vec4 WHITE(1.0f, 1.0f, 1.0f, 1.0f);
        const float AXIS_LENGTH = 0.1f;
    
        AnimPoseVec absoluteJointPoses;
        AnimPose rigToAvatar = AnimPose(Quaternions::Y_180 * getWorldOrientation(), getWorldPosition());
        bool drawBones = false;
        for (int i = 0; i < jointData.size(); i++) {
            float jointScale = skeletonData[i].defaultScale * getTargetScale() * METERS_PER_CENTIMETER;
            auto absoluteRotation = jointData[i].rotationIsDefaultPose ? skeletonData[i].defaultRotation : jointData[i].rotation;
            auto localJointTranslation = jointScale * (jointData[i].translationIsDefaultPose ? skeletonData[i].defaultTranslation : jointData[i].translation);
            bool isHips = skeletonData[i].jointName == "Hips";
            if (isHips) { 
                localJointTranslation = glm::vec3(0.0f);
                drawBones = true;
            }
            AnimPose absoluteParentPose;
            int parentIndex = skeletonData[i].parentIndex;
            if (parentIndex != -1 && parentIndex < (int)absoluteJointPoses.size()) {
                absoluteParentPose = absoluteJointPoses[parentIndex];
            }
            AnimPose absoluteJointPose = AnimPose(absoluteRotation, absoluteParentPose.trans() + absoluteParentPose.rot() * localJointTranslation);
            auto jointPose = rigToAvatar * absoluteJointPose;
            auto parentPose = rigToAvatar * absoluteParentPose;
            if (drawBones) {
                glm::vec3 xAxis = jointPose.rot() * Vectors::UNIT_X;
                glm::vec3 yAxis = jointPose.rot() * Vectors::UNIT_Y;
                glm::vec3 zAxis = jointPose.rot() * Vectors::UNIT_Z;

                DebugDraw::getInstance().drawRay(jointPose.trans(), jointPose.trans() + AXIS_LENGTH * xAxis, jointData[i].rotationIsDefaultPose ? LIGHT_RED : RED);
                DebugDraw::getInstance().drawRay(jointPose.trans(), jointPose.trans() + AXIS_LENGTH * yAxis, jointData[i].rotationIsDefaultPose ? LIGHT_GREEN : GREEN);
                DebugDraw::getInstance().drawRay(jointPose.trans(), jointPose.trans() + AXIS_LENGTH * zAxis, jointData[i].rotationIsDefaultPose ? LIGHT_BLUE : BLUE);
                if (!isHips) {
                    DebugDraw::getInstance().drawRay(jointPose.trans(), parentPose.trans(), jointData[i].translationIsDefaultPose ? WHITE : GREY);
                }
            }
            absoluteJointPoses.push_back(absoluteJointPose);
        }
    }
}

void OtherAvatar::handleChangedAvatarEntityData() {
    PerformanceTimer perfTimer("attachments");

    // AVATAR ENTITY UPDATE FLOW
    // - if queueEditEntityMessage() sees "AvatarEntity" HostType it calls _myAvatar->storeAvatarEntityDataPayload()
    // - storeAvatarEntityDataPayload() saves the payload and flags the trait instance for the entity as updated,
    // - ClientTraitsHandler::sendChangedTraitsToMixer() sends the entity bytes to the mixer which relays them to other interfaces
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
            // FIXME: This function will cause unintented changes in SpaillyNestable
            // E.g overriding the ID index of an exisiting entity to temporary entity
            // in the following map QHash<QUuid, SpatiallyNestableWeakPointer> _children;
            // Andrew Meadows will address this issue
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

                // Since  has overwrtiiten the back pointer
                // from the parent children map (see comment for function call above),
                // we need to for reset the back pointer in the map correctly by setting the parentID, but
                // since the parentID of the entity has not changed we first need to set it some ither ID,
                // then set the the original ID for the changes to take effect
                // TODO: This is a horrible hack and once properties.constructFromBuffer no longer causes
                // side effects...remove the following three lines

                const QUuid NULL_ID = QUuid("{00000000-0000-0000-0000-000000000005}");
                entity->setParentID(NULL_ID);
                entity->setParentID(oldParentID);

                if (entity->stillHasMyGrab()) {
                    // For this case: we want to ignore transform+velocities coming from authoritative OtherAvatar
                    // because the MyAvatar is grabbing and we expect the local grab state
                    // to have enough information to prevent simulation drift.
                    //
                    // Clever readers might realize this could cause problems.  For example,
                    // if an ignored OtherAvatar were to simultanously grab the object then there would be
                    // a noticeable discrepancy between participants in the distributed physics simulation,
                    // however the difference would be stable and would not drift.
                    properties.clearTransformOrVelocityChanges();
                }
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
            if (success) {
                stateItr.value().hash = newHash;
            } else {
                stateItr.value().hash = 0;
            }
        }

        AvatarEntityIDs recentlyRemovedAvatarEntities = getAndClearRecentlyRemovedIDs();
        if (!recentlyRemovedAvatarEntities.empty()) {
            // only lock this thread when absolutely necessary
            AvatarEntityMap packedAvatarEntityData;
            _avatarEntitiesLock.withReadLock([&] {
                packedAvatarEntityData = _packedAvatarEntityData;
            });
            if (!recentlyRemovedAvatarEntities.empty()) {
                std::vector<EntityItemID> idsToDelete;
                idsToDelete.reserve(recentlyRemovedAvatarEntities.size());
                foreach (auto entityID, recentlyRemovedAvatarEntities) {
                    if (!packedAvatarEntityData.contains(entityID)) {
                        idsToDelete.push_back(entityID);
                    }
                }
                if (!idsToDelete.empty()) {
                    bool force = true;
                    bool ignoreWarnings = true;
                    entityTree->deleteEntitiesByID(idsToDelete, force, ignoreWarnings);
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
