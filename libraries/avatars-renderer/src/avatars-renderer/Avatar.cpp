//
//  Avatar.cpp
//  interface/src/avatar
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Avatar.h"

#include <QtCore/QThread>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_query.hpp>

#include <AvatarConstants.h>
#include <shared/QtHelpers.h>
#include <DeferredLightingEffect.h>
#include <EntityTreeRenderer.h>
#include <NodeList.h>
#include <NumericalConstants.h>
#include <OctreeUtils.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>
#include <Rig.h>
#include <SharedUtil.h>
#include <TextRenderer3D.h>
#include <VariantMapToScriptValue.h>
#include <DebugDraw.h>
#include <shared/Camera.h>
#include <SoftAttachmentModel.h>
#include <render/TransitionStage.h>
#include <GLMHelpers.h>
#include "ModelEntityItem.h"
#include "RenderableModelEntityItem.h"

#include <graphics-scripting/Forward.h>
#include <CubicHermiteSpline.h>

#include "Logging.h"

using namespace std;

const float CHAT_MESSAGE_SCALE = 0.0015f;
const float CHAT_MESSAGE_HEIGHT = 0.1f;
const float DISPLAYNAME_FADE_TIME = 0.5f;
const float DISPLAYNAME_FADE_FACTOR = pow(0.01f, 1.0f / DISPLAYNAME_FADE_TIME);
const float DISPLAYNAME_ALPHA = 1.0f;
const float DISPLAYNAME_BACKGROUND_ALPHA = 0.4f;
const glm::vec3 HAND_TO_PALM_OFFSET(0.0f, 0.12f, 0.08f);
const float Avatar::MYAVATAR_LOADING_PRIORITY = (float)M_PI; // Entity priority is computed as atan2(maxDim, distance) which is <= PI / 2
const float Avatar::OTHERAVATAR_LOADING_PRIORITY = MYAVATAR_LOADING_PRIORITY - EPSILON;
const float Avatar::ATTACHMENT_LOADING_PRIORITY = OTHERAVATAR_LOADING_PRIORITY - EPSILON;

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar) {
        ItemKey::Builder keyBuilder = ItemKey::Builder::opaqueShape().withTypeMeta().withTagBits(render::hifi::TAG_ALL_VIEWS).withMetaCullGroup();
        auto avatarPtr = static_pointer_cast<Avatar>(avatar);
        if (!avatarPtr->getEnableMeshVisible()) {
            keyBuilder.withInvisible();
        }
        return keyBuilder.build();
    }
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar, RenderArgs* args) {
        auto avatarPtr = static_pointer_cast<Avatar>(avatar);
        if (avatarPtr) {
            return avatarPtr->getRenderBounds();
        }
        return Item::Bound();
    }
    template <> void payloadRender(const AvatarSharedPointer& avatar, RenderArgs* args) {
        auto avatarPtr = static_pointer_cast<Avatar>(avatar);
        if (avatarPtr->isInitialized() && args) {
            PROFILE_RANGE_BATCH(*args->_batch, "renderAvatarPayload");
            avatarPtr->render(args);
        }
    }
    template <> uint32_t metaFetchMetaSubItems(const AvatarSharedPointer& avatar, ItemIDs& subItems) {
        auto avatarPtr = static_pointer_cast<Avatar>(avatar);
        if (avatarPtr) {
            uint32_t total = 0;
            if (avatarPtr->getSkeletonModel()) {
                auto& metaSubItems = avatarPtr->getSkeletonModel()->fetchRenderItemIDs();
                subItems.insert(subItems.end(), metaSubItems.begin(), metaSubItems.end());
                total += (uint32_t)metaSubItems.size();
            }
            total += avatarPtr->appendSubMetaItems(subItems);
            return total;
        }
        return 0;
    }
}

bool showAvatars { true };
void Avatar::setShowAvatars(bool render) {
    showAvatars = render;
}

static bool showReceiveStats = false;
void Avatar::setShowReceiveStats(bool receiveStats) {
    showReceiveStats = receiveStats;
}

static bool showMyLookAtVectors = false;
void Avatar::setShowMyLookAtVectors(bool showMine) {
    showMyLookAtVectors = showMine;
}

static bool showMyLookAtTarget = false;
void Avatar::setShowMyLookAtTarget(bool showMine) {
    showMyLookAtTarget = showMine;
}

static bool showOtherLookAtVectors = false;
void Avatar::setShowOtherLookAtVectors(bool showOthers) {
    showOtherLookAtVectors = showOthers;
}

static bool showOtherLookAtTarget = false;
void Avatar::setShowOtherLookAtTarget(bool showOthers) {
    showOtherLookAtTarget = showOthers;
}

static bool showCollisionShapes = false;
void Avatar::setShowCollisionShapes(bool render) {
    showCollisionShapes = render;
}

static bool showNamesAboveHeads = false;
void Avatar::setShowNamesAboveHeads(bool show) {
    showNamesAboveHeads = show;
}

AvatarTransit::Status AvatarTransit::update(float deltaTime, const glm::vec3& avatarPosition, const AvatarTransit::TransitConfig& config) {
    float oneFrameDistance = _isActive ? glm::length(avatarPosition - _endPosition) : glm::length(avatarPosition - _lastPosition);
    if (oneFrameDistance > (config._minTriggerDistance * _scale)) {
        if (oneFrameDistance < (config._maxTriggerDistance * _scale)) {
            start(deltaTime, _lastPosition, avatarPosition, config);
        } else {
            _lastPosition = avatarPosition;
            _status = Status::ABORT_TRANSIT;
        }
    }
    _lastPosition = avatarPosition;
    _status = updatePosition(deltaTime);

    if (_isActive && oneFrameDistance > (config._abortDistance * _scale) && _status == Status::POST_TRANSIT) {
        reset();
        _status = Status::ENDED;
    }

    return _status;
}

void AvatarTransit::slamPosition(const glm::vec3& avatarPosition) {
    // used to instantly teleport between two points without triggering a change in status.
    _lastPosition = avatarPosition;
    _endPosition = avatarPosition;
}

void AvatarTransit::reset() {
    _lastPosition = _endPosition;
    _currentPosition = _endPosition;
    _isActive = false;
}

void AvatarTransit::start(float deltaTime, const glm::vec3& startPosition, const glm::vec3& endPosition, const AvatarTransit::TransitConfig& config) {
    _startPosition = startPosition;
    _endPosition = endPosition;

    _transitLine = endPosition - startPosition;
    _totalDistance = glm::length(_transitLine);
    _easeType = config._easeType;

    _preTransitTime = AVATAR_PRE_TRANSIT_FRAME_COUNT / AVATAR_TRANSIT_FRAMES_PER_SECOND;
    _postTransitTime = AVATAR_POST_TRANSIT_FRAME_COUNT / AVATAR_TRANSIT_FRAMES_PER_SECOND;
    int transitFrames = (!config._isDistanceBased) ? config._totalFrames : config._framesPerMeter * _totalDistance;
    _transitTime = (float)transitFrames / AVATAR_TRANSIT_FRAMES_PER_SECOND;
    _totalTime = _transitTime + _preTransitTime + _postTransitTime;
    _currentTime = _isActive ? _preTransitTime : 0.0f;
    _isActive = true;
}

float AvatarTransit::getEaseValue(AvatarTransit::EaseType type, float value) {
    switch (type) {
    case EaseType::NONE:
        return value;
        break;
    case EaseType::EASE_IN:
        return value * value;
        break;
    case EaseType::EASE_OUT:
        return value * (2.0f - value);
        break;
    case EaseType::EASE_IN_OUT:
        return (value < 0.5f) ? 2.0f * value * value : -1.0f + (4.0f - 2.0f * value) * value;
        break;
    }
    return value;
}

AvatarTransit::Status AvatarTransit::updatePosition(float deltaTime) {
    Status status = Status::IDLE;
    if (_isActive) {
        float nextTime = _currentTime + deltaTime;
        if (nextTime < _preTransitTime) {
            _currentPosition = _startPosition;
            status = Status::PRE_TRANSIT;
            if (_currentTime == 0) {
                status = Status::STARTED;
            }
        } else if (nextTime < _totalTime - _postTransitTime) {
            status = Status::TRANSITING;
            if (_currentTime <= _preTransitTime) {
                status = Status::START_TRANSIT;
            } else {
                float percentageIntoTransit = (nextTime - _preTransitTime) / _transitTime;
                _currentPosition = _startPosition + getEaseValue(_easeType, percentageIntoTransit) * _transitLine;
            }
        } else {
            status = Status::POST_TRANSIT;
            _currentPosition = _endPosition;
            if (nextTime >= _totalTime) {
                _isActive = false;
                status = Status::ENDED;
            } else if (_currentTime < _totalTime - _postTransitTime) {
                status = Status::END_TRANSIT;
            }
        }
        _currentTime = nextTime;
    }
    return status;
}

Avatar::Avatar(QThread* thread) :
    _voiceSphereID(GeometryCache::UNKNOWN_ID)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(thread);

    setModelScale(1.0f); // avatar scale is uniform

    auto geometryCache = DependencyManager::get<GeometryCache>();
    _nameRectGeometryID = geometryCache->allocateID();
    _leftPointerGeometryID = geometryCache->allocateID();
    _rightPointerGeometryID = geometryCache->allocateID();
    _lastRenderUpdateTime = usecTimestampNow();

    indicateLoadingStatus(LoadingStatus::NoModel);
}

Avatar::~Avatar() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_nameRectGeometryID);
        geometryCache->releaseID(_leftPointerGeometryID);
        geometryCache->releaseID(_rightPointerGeometryID);
    }
}

void Avatar::init() {
    getHead()->init();
    _initialized = true;
}

glm::vec3 Avatar::getChestPosition() const {
    // for now, let's just assume that the "chest" is halfway between the root and the neck
    glm::vec3 neckPosition;
    return _skeletonModel->getNeckPosition(neckPosition) ? (getWorldPosition() + neckPosition) * 0.5f : getWorldPosition();
}

glm::vec3 Avatar::getNeckPosition() const {
    glm::vec3 neckPosition;
    return _skeletonModel->getNeckPosition(neckPosition) ? neckPosition : getWorldPosition();
}

AABox Avatar::getBounds() const {
    if (!_skeletonModel->isRenderable() || _skeletonModel->needsFixupInScene()) {
        // approximately 2m tall, scaled to user request.
        return AABox(getWorldPosition() - glm::vec3(getModelScale()), getModelScale() * 2.0f);
    }
    return _skeletonModel->getRenderableMeshBound();
}


AABox Avatar::getRenderBounds() const {
    return _renderBound;
}

void Avatar::animateScaleChanges(float deltaTime) {

    if (_isAnimatingScale) {
        float currentScale = getModelScale();
        float desiredScale = getDomainLimitedScale();

        // use exponential decay toward the domain limit clamped scale
        const float SCALE_ANIMATION_TIMESCALE = 0.5f;
        float blendFactor = glm::clamp(deltaTime / SCALE_ANIMATION_TIMESCALE, 0.0f, 1.0f);
        float animatedScale = (1.0f - blendFactor) * currentScale + blendFactor * desiredScale;

        // snap to the end when we get close enough
        const float MIN_RELATIVE_ERROR = 0.001f;
        float relativeError = fabsf(desiredScale - currentScale) / desiredScale;
        if (relativeError < MIN_RELATIVE_ERROR) {
            animatedScale = desiredScale;
            _isAnimatingScale = false;
        }
        setModelScale(animatedScale); // avatar scale is uniform

        // flag the joints as having changed for force update to RenderItem
        _hasNewJointData = true;

        // TODO: rebuilding the shape constantly is somehwat expensive.
        // We should only rebuild after significant change.
        rebuildCollisionShape();
    }
}

void Avatar::setTargetScale(float targetScale) {
    float newValue = glm::clamp(targetScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);
    if (_targetScale != newValue) {
        _targetScale = newValue;
        _scaleChanged = usecTimestampNow();
        _avatarScaleChanged = _scaleChanged;
        _isAnimatingScale = true;
        for (auto& sphere : _multiSphereShapes) {
            sphere.setScale(_targetScale);
        }
        emit targetScaleChanged(targetScale);
    }
}

void Avatar::removeAvatarEntitiesFromTree() {
    if (_packedAvatarEntityData.empty()) {
        return;
    }
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (entityTree) {
        std::vector<EntityItemID> ids;
        ids.reserve(_packedAvatarEntityData.size());
        PackedAvatarEntityMap::const_iterator itr = _packedAvatarEntityData.constBegin();
        while (itr != _packedAvatarEntityData.constEnd()) {
            ids.push_back(itr.key());
            ++itr;
        }
        bool force = true;
        bool ignoreWarnings = true;
        entityTree->deleteEntitiesByID(ids, force, ignoreWarnings); // locks tree
    }
}

bool Avatar::applyGrabChanges() {
    if (!_avatarGrabDataChanged && _grabsToChange.empty() && _grabsToDelete.empty()) {
        // early exit for most common case: nothing to do
        return false;
    }

    bool grabAddedOrRemoved = false;
    _avatarGrabsLock.withWriteLock([&] {
        if (_avatarGrabDataChanged) {
            // collect changes in _avatarGrabData
            foreach (auto grabID, _avatarGrabData.keys()) {
                MapOfGrabs::iterator itr = _avatarGrabs.find(grabID);
                if (itr == _avatarGrabs.end()) {
                    GrabPointer grab = std::make_shared<Grab>();
                    grab->fromByteArray(_avatarGrabData.value(grabID));
                    _avatarGrabs[grabID] = grab;
                    _grabsToChange.insert(grabID);
                } else {
                    bool changed = itr->second->fromByteArray(_avatarGrabData.value(grabID));
                    if (changed) {
                        _grabsToChange.insert(grabID);
                    }
                }
            }
            _avatarGrabDataChanged = false;
        }

        // delete _avatarGrabs
        VectorOfIDs undeleted;
        for (const auto& id : _grabsToDelete) {
            MapOfGrabs::iterator itr = _avatarGrabs.find(id);
            if (itr == _avatarGrabs.end()) {
                continue;
            }

            bool success;
            const GrabPointer& grab = itr->second;
            SpatiallyNestablePointer target = SpatiallyNestable::findByID(grab->getTargetID(), success);
            if (success && target) {
                target->removeGrab(grab);
                _avatarGrabs.erase(itr);
                grabAddedOrRemoved = true;
            } else {
                undeleted.push_back(id);
            }
        }
        _grabsToDelete = std::move(undeleted);

        // change _avatarGrabs and add Actions to target
        SetOfIDs unchanged;
        for (const auto& id : _grabsToChange) {
            MapOfGrabs::iterator itr = _avatarGrabs.find(id);
            if (itr == _avatarGrabs.end()) {
                continue;
            }

            bool success;
            const GrabPointer& grab = itr->second;
            SpatiallyNestablePointer target = SpatiallyNestable::findByID(grab->getTargetID(), success);
            if (success && target) {
                target->addGrab(grab);
                if (isMyAvatar()) {
                    EntityItemPointer entity = std::dynamic_pointer_cast<EntityItem>(target);
                    if (entity) {
                        entity->upgradeScriptSimulationPriority(PERSONAL_SIMULATION_PRIORITY);
                    }
                }
                grabAddedOrRemoved = true;
            } else {
                unchanged.insert(id);
            }
        }
        _grabsToChange = std::move(unchanged);
    });
    return grabAddedOrRemoved;
}

void Avatar::accumulateGrabPositions(std::map<QUuid, GrabLocationAccumulator>& grabAccumulators) {
    // relay avatar's joint position to grabbed target in a way that allows for averaging
    _avatarGrabsLock.withReadLock([&] {
        for (const auto& entry : _avatarGrabs) {
            const GrabPointer& grab = entry.second;

            if (!grab || !grab->getActionID().isNull()) {
                continue; // the accumulated value isn't used, in this case.
            }
            if (grab->getReleased()) {
                continue;
            }

            glm::vec3 jointTranslation = getAbsoluteJointTranslationInObjectFrame(grab->getParentJointIndex());
            glm::quat jointRotation = getAbsoluteJointRotationInObjectFrame(grab->getParentJointIndex());
            glm::mat4 jointMat = createMatFromQuatAndPos(jointRotation, jointTranslation);
            glm::mat4 offsetMat = createMatFromQuatAndPos(grab->getRotationalOffset(), grab->getPositionalOffset());
            glm::mat4 avatarMat = getTransform().getMatrix();
            glm::mat4 worldTransform = avatarMat * jointMat * offsetMat;
            GrabLocationAccumulator& grabLocationAccumulator = grabAccumulators[grab->getTargetID()];
            grabLocationAccumulator.accumulate(extractTranslation(worldTransform), extractRotation(worldTransform));
        }
    });
}

void Avatar::tearDownGrabs() {
    _avatarGrabsLock.withWriteLock([&] {
        for (const auto& entry : _avatarGrabs) {
            _grabsToDelete.push_back(entry.first);
        }
        _grabsToChange.clear();
    });
    applyGrabChanges();
    if (!_grabsToDelete.empty()) {
        // some grabs failed to delete, which is a possible "leak", so we log about it
        qWarning() << "Failed to tearDown" << _grabsToDelete.size() << "grabs for Avatar" << getID();
    }
}

void Avatar::relayJointDataToChildren() {
    forEachChild([&](SpatiallyNestablePointer child) {
        if (child->getNestableType() == NestableType::Entity) {
            auto  modelEntity = std::dynamic_pointer_cast<RenderableModelEntityItem>(child);
            if (modelEntity) {
                if (modelEntity->getRelayParentJoints()) {
                    if (!modelEntity->getJointMapCompleted() || _reconstructSoftEntitiesJointMap) {
                        QStringList modelJointNames = modelEntity->getJointNames();
                        int numJoints = modelJointNames.count();
                        std::vector<int> map;
                        map.reserve(numJoints);
                        for (int jointIndex = 0; jointIndex < numJoints; jointIndex++)  {
                            QString jointName = modelJointNames.at(jointIndex);
                            int avatarJointIndex = getJointIndex(jointName);
                            glm::quat jointRotation;
                            glm::vec3 jointTranslation;
                            if (avatarJointIndex < 0) {
                                jointRotation = modelEntity->getLocalJointRotation(jointIndex);
                                jointTranslation = modelEntity->getLocalJointTranslation(jointIndex);
                                map.push_back(-1);
                            } else {
                                int jointIndex = getJointIndex(jointName);
                                jointRotation = getJointRotation(jointIndex);
                                jointTranslation = getJointTranslation(jointIndex);
                                map.push_back(avatarJointIndex);
                            }
                            modelEntity->setLocalJointRotation(jointIndex, jointRotation);
                            modelEntity->setLocalJointTranslation(jointIndex, jointTranslation);
                        }
                        modelEntity->setJointMap(map);
                    } else {
                        QStringList modelJointNames = modelEntity->getJointNames();
                        int numJoints = modelJointNames.count();
                        for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
                            int avatarJointIndex = modelEntity->avatarJointIndex(jointIndex);
                            glm::quat jointRotation;
                            glm::vec3 jointTranslation;
                            if (avatarJointIndex >=0) {
                                jointRotation = getJointRotation(avatarJointIndex);
                                jointTranslation = getJointTranslation(avatarJointIndex);
                            } else {
                                jointRotation = modelEntity->getLocalJointRotation(jointIndex);
                                jointTranslation = modelEntity->getLocalJointTranslation(jointIndex);
                            }
                            modelEntity->setLocalJointRotation(jointIndex, jointRotation);
                            modelEntity->setLocalJointTranslation(jointIndex, jointTranslation);
                        }
                    }

                    Transform finalTransform;
                    Transform avatarTransform = _skeletonModel->getTransform();
                    Transform entityTransform = modelEntity->getLocalTransform();
                    Transform::mult(finalTransform, avatarTransform, entityTransform);
                    finalTransform.setScale(_skeletonModel->getScale());
                    modelEntity->setOverrideTransform(finalTransform, _skeletonModel->getOffset());
                    modelEntity->simulateRelayedJoints();
                }
            }
        }
    });
    _reconstructSoftEntitiesJointMap = false;
}

/*@jsdoc
 * <p>An avatar has different types of data simulated at different rates, in Hz.</p>
 *
 * <table>
 *   <thead>
 *     <tr><th>Rate Name</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"avatar" or ""</code></td><td>The rate at which the avatar is updated even if not in view.</td></tr>
 *     <tr><td><code>"avatarInView"</code></td><td>The rate at which the avatar is updated if in view.</td></tr>
 *     <tr><td><code>"skeletonModel"</code></td><td>The rate at which the skeleton model is being updated, even if there are no
 *       joint data available.</td></tr>
 *     <tr><td><code>"jointData"</code></td><td>The rate at which joint data are being updated.</td></tr>
 *     <tr><td><code>""</code></td><td>When no rate name is specified, the <code>"avatar"</code> update rate is
 *       provided.</td></tr>
 *   </tbody>
 * </table>
 *
 * @typedef {string} AvatarSimulationRate
 */
float Avatar::getSimulationRate(const QString& rateName) const {
    if (rateName == "") {
        return _simulationRate.rate();
    } else if (rateName == "avatar") {
        return _simulationRate.rate();
    } else if (rateName == "avatarInView") {
        return _simulationInViewRate.rate();
    } else if (rateName == "skeletonModel") {
        return _skeletonModelSimulationRate.rate();
    } else if (rateName == "jointData") {
        return _jointDataSimulationRate.rate();
    }
    return 0.0f;
}

bool Avatar::isLookingAtMe(AvatarSharedPointer avatar) const {
    const float HEAD_SPHERE_RADIUS = 0.1f;
    glm::vec3 theirLookAt = dynamic_pointer_cast<Avatar>(avatar)->getHead()->getLookAtPosition();
    glm::vec3 myEyePosition = getHead()->getEyePosition();

    return glm::distance(theirLookAt, myEyePosition) <= (HEAD_SPHERE_RADIUS * getModelScale());
}

void Avatar::slamPosition(const glm::vec3& newPosition) {
    setWorldPosition(newPosition);
    _positionDeltaAccumulator = glm::vec3(0.0f);
    setWorldVelocity(glm::vec3(0.0f));
    _lastVelocity = glm::vec3(0.0f);
    _transit.slamPosition(newPosition);
}

void Avatar::updateAttitude(const glm::quat& orientation) {
    _skeletonModel->updateAttitude(orientation);
    _worldUpDirection = orientation * Vectors::UNIT_Y;
}

void Avatar::applyPositionDelta(const glm::vec3& delta) {
    setWorldPosition(getWorldPosition() + delta);
    _positionDeltaAccumulator += delta;
}

void Avatar::measureMotionDerivatives(float deltaTime) {
    PerformanceTimer perfTimer("derivatives");
    // linear
    const float MIN_DELTA_TIME = 0.001f;
    const float safeDeltaTime = glm::max(deltaTime, MIN_DELTA_TIME);
    float invDeltaTime = 1.0f / safeDeltaTime;
    // Floating point error prevents us from computing velocity in a naive way
    // (e.g. vel = (pos - oldPos) / dt) so instead we use _positionOffsetAccumulator.
    glm::vec3 velocity = _positionDeltaAccumulator * invDeltaTime;
    _positionDeltaAccumulator = glm::vec3(0.0f);
    _acceleration = (velocity - _lastVelocity) * invDeltaTime;
    _lastVelocity = velocity;
    setWorldVelocity(velocity);

    // angular
    glm::quat orientation = getWorldOrientation();
    float changeDot = glm::abs(glm::dot(orientation, _lastOrientation));
    float CHANGE_DOT_THRESHOLD = 0.9999f;
    if (changeDot < CHANGE_DOT_THRESHOLD) {
        float angle = 2.0f * acosf(changeDot);
        glm::quat delta = glm::inverse(_lastOrientation) * orientation;
        glm::vec3 angularVelocity = (angle * invDeltaTime) * glm::axis(delta);
        setWorldAngularVelocity(angularVelocity);
        _lastOrientation = orientation;
    } else {
        setWorldAngularVelocity(glm::vec3(0.0f));
    }
}

void Avatar::metaBlendshapeOperator(render::ItemID renderItemID, int blendshapeNumber, const QVector<BlendshapeOffset>& blendshapeOffsets,
                                    const QVector<int>& blendedMeshSizes, const render::ItemIDs& subItemIDs) {
    render::Transaction transaction;
    transaction.updateItem<AvatarData>(renderItemID, [blendshapeNumber, blendshapeOffsets, blendedMeshSizes,
                                                       subItemIDs](AvatarData& avatar) {
        auto avatarPtr = dynamic_cast<Avatar*>(&avatar);
        if (avatarPtr) {
            avatarPtr->setBlendedVertices(blendshapeNumber, blendshapeOffsets, blendedMeshSizes, subItemIDs);
        }
    });
    AbstractViewStateInterface::instance()->getMain3DScene()->enqueueTransaction(transaction);
}

void Avatar::addToScene(AvatarSharedPointer self, const render::ScenePointer& scene, render::Transaction& transaction) {
    auto avatarPayload = new render::Payload<AvatarData>(self);
    auto avatarPayloadPointer = std::shared_ptr<render::Payload<AvatarData>>(avatarPayload);
    if (_renderItemID == render::Item::INVALID_ITEM_ID) {
        _renderItemID = scene->allocateID();
    }
    // INitialize the _render bound as we are creating the avatar render item
    _renderBound = getBounds();
    transaction.resetItem(_renderItemID, avatarPayloadPointer);
    using namespace std::placeholders;
    _skeletonModel->addToScene(scene, transaction, std::bind(&Avatar::metaBlendshapeOperator, _renderItemID, _1, _2, _3, _4));
    _skeletonModel->setTagMask(render::hifi::TAG_ALL_VIEWS);
    _skeletonModel->setGroupCulled(true);
    _skeletonModel->setCanCastShadow(true);
    _skeletonModel->setVisibleInScene(_isMeshVisible, scene);

    processMaterials();
    bool attachmentRenderingNeedsUpdate = false;
    for (auto& attachmentModel : _attachmentModels) {
        attachmentModel->addToScene(scene, transaction);
        attachmentModel->setTagMask(render::hifi::TAG_ALL_VIEWS);
        attachmentModel->setGroupCulled(true);
        attachmentModel->setCanCastShadow(true);
        attachmentModel->setVisibleInScene(_isMeshVisible, scene);
        attachmentRenderingNeedsUpdate = true;
    }

    if (attachmentRenderingNeedsUpdate) {
        updateAttachmentRenderIDs();
    }

    _mustFadeIn = true;
    emit DependencyManager::get<scriptable::ModelProviderFactory>()->modelAddedToScene(getSessionUUID(), NestableType::Avatar, _skeletonModel);
}

void Avatar::fadeIn(render::ScenePointer scene) {
    render::Transaction transaction;
    fade(transaction, render::Transition::USER_ENTER_DOMAIN);
    scene->enqueueTransaction(transaction);
}

void Avatar::fadeOut(render::Transaction& transaction, KillAvatarReason reason) {
    render::Transition::Type transitionType = render::Transition::USER_LEAVE_DOMAIN;

    if (reason == KillAvatarReason::YourAvatarEnteredTheirBubble) {
        transitionType = render::Transition::BUBBLE_ISECT_TRESPASSER;
    } else if (reason == KillAvatarReason::TheirAvatarEnteredYourBubble) {
        transitionType = render::Transition::BUBBLE_ISECT_OWNER;
    }
    fade(transaction, transitionType);
}

void Avatar::fade(render::Transaction& transaction, render::Transition::Type type) {
    transaction.resetTransitionOnItem(_renderItemID, type);
    for (auto& attachmentModel : _attachmentModels) {
        for (auto itemId : attachmentModel->fetchRenderItemIDs()) {
            transaction.resetTransitionOnItem(itemId, type, _renderItemID);
        }
    }
    _lastFadeRequested = type;
}

render::Transition::Type Avatar::getLastFadeRequested() const {
    return _lastFadeRequested;
}

void Avatar::removeFromScene(AvatarSharedPointer self, const render::ScenePointer& scene, render::Transaction& transaction) {
    transaction.removeItem(_renderItemID);
    render::Item::clearID(_renderItemID);
    _skeletonModel->removeFromScene(scene, transaction);
    for (auto& attachmentModel : _attachmentModels) {
        attachmentModel->removeFromScene(scene, transaction);
    }
    emit DependencyManager::get<scriptable::ModelProviderFactory>()->modelRemovedFromScene(getSessionUUID(), NestableType::Avatar, _skeletonModel);
}

void Avatar::updateRenderItem(render::Transaction& transaction) {
    if (render::Item::isValidID(_renderItemID)) {
        auto renderBound = getBounds();
        transaction.updateItem<AvatarData>(_renderItemID,
            [renderBound](AvatarData& avatar) {
                auto avatarPtr = dynamic_cast<Avatar*>(&avatar);
                if (avatarPtr) {
                    avatarPtr->_renderBound = renderBound;
                }
            }
        );
    }
}

void Avatar::postUpdate(float deltaTime, const render::ScenePointer& scene) {

    if (isMyAvatar() ? showMyLookAtTarget : showOtherLookAtTarget) {
        glm::vec3 lookAtTarget = getHead()->getLookAtPosition();
        DebugDraw::getInstance().addMarker(QString("look-at-") + getID().toString(),
                                           glm::quat(), lookAtTarget, glm::vec4(1), 1.0f);
    } else {
        DebugDraw::getInstance().removeMarker(QString("look-at-") + getID().toString());
    }

    if (isMyAvatar() ? showMyLookAtVectors : showOtherLookAtVectors) {
        const float EYE_RAY_LENGTH = 10.0;
        const glm::vec4 BLUE(0.0f, 0.0f, _lookAtSnappingEnabled ? 1.0f : 0.25f, 1.0f);
        const glm::vec4 RED(_lookAtSnappingEnabled ? 1.0f : 0.25f, 0.0f, 0.0f, 1.0f);

        int leftEyeJoint = getJointIndex("LeftEye");
        glm::vec3 leftEyePosition;
        glm::quat leftEyeRotation;

        if (_skeletonModel->getJointPositionInWorldFrame(leftEyeJoint, leftEyePosition) &&
            _skeletonModel->getJointRotationInWorldFrame(leftEyeJoint, leftEyeRotation)) {
            DebugDraw::getInstance().drawRay(leftEyePosition, leftEyePosition + leftEyeRotation * Vectors::UNIT_Z * EYE_RAY_LENGTH, BLUE);
        }

        int rightEyeJoint = getJointIndex("RightEye");
        glm::vec3 rightEyePosition;
        glm::quat rightEyeRotation;
        if (_skeletonModel->getJointPositionInWorldFrame(rightEyeJoint, rightEyePosition) &&
            _skeletonModel->getJointRotationInWorldFrame(rightEyeJoint, rightEyeRotation)) {
            DebugDraw::getInstance().drawRay(rightEyePosition, rightEyePosition + rightEyeRotation * Vectors::UNIT_Z * EYE_RAY_LENGTH, RED);
        }
    }

    fixupModelsInScene(scene);
    updateFitBoundingBox();
}

void Avatar::render(RenderArgs* renderArgs) {
    auto& batch = *(renderArgs->_batch);
    PROFILE_RANGE_BATCH(batch, __FUNCTION__);

    glm::vec3 viewPos = renderArgs->getViewFrustum().getPosition();
    const float MAX_DISTANCE_SQUARED_FOR_SHOWING_POINTING_LASERS = 100.0f; // 10^2
    if (glm::distance2(viewPos, getWorldPosition()) < MAX_DISTANCE_SQUARED_FOR_SHOWING_POINTING_LASERS) {
        auto geometryCache = DependencyManager::get<GeometryCache>();

        // render pointing lasers
        glm::vec3 laserColor = glm::vec3(1.0f, 0.0f, 1.0f);
        float laserLength = 50.0f;
        glm::vec3 position;
        glm::quat rotation;
        bool havePosition, haveRotation;

        if (_handState & LEFT_HAND_POINTING_FLAG) {
            if (_handState & IS_FINGER_POINTING_FLAG) {
                int leftIndexTip = getJointIndex("LeftHandIndex4");
                int leftIndexTipJoint = getJointIndex("LeftHandIndex3");
                havePosition = _skeletonModel->getJointPositionInWorldFrame(leftIndexTip, position);
                haveRotation = _skeletonModel->getJointRotationInWorldFrame(leftIndexTipJoint, rotation);
            } else {
                int leftHand = _skeletonModel->getLeftHandJointIndex();
                havePosition = _skeletonModel->getJointPositionInWorldFrame(leftHand, position);
                haveRotation = _skeletonModel->getJointRotationInWorldFrame(leftHand, rotation);
            }

            if (havePosition && haveRotation) {
                PROFILE_RANGE_BATCH(batch, __FUNCTION__":leftHandPointer");
                Transform pointerTransform;
                pointerTransform.setTranslation(position);
                pointerTransform.setRotation(rotation);
                batch.setModelTransform(pointerTransform);
                geometryCache->bindSimpleProgram(batch, false, false, false, false, true, renderArgs->_renderMethod == render::Args::FORWARD);
                geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, laserLength, 0.0f), laserColor, _leftPointerGeometryID);
            }
        }

        if (_handState & RIGHT_HAND_POINTING_FLAG) {

            if (_handState & IS_FINGER_POINTING_FLAG) {
                int rightIndexTip = getJointIndex("RightHandIndex4");
                int rightIndexTipJoint = getJointIndex("RightHandIndex3");
                havePosition = _skeletonModel->getJointPositionInWorldFrame(rightIndexTip, position);
                haveRotation = _skeletonModel->getJointRotationInWorldFrame(rightIndexTipJoint, rotation);
            } else {
                int rightHand = _skeletonModel->getRightHandJointIndex();
                havePosition = _skeletonModel->getJointPositionInWorldFrame(rightHand, position);
                haveRotation = _skeletonModel->getJointRotationInWorldFrame(rightHand, rotation);
            }

            if (havePosition && haveRotation) {
                PROFILE_RANGE_BATCH(batch, __FUNCTION__":rightHandPointer");
                Transform pointerTransform;
                pointerTransform.setTranslation(position);
                pointerTransform.setRotation(rotation);
                batch.setModelTransform(pointerTransform);
                geometryCache->bindSimpleProgram(batch, false, false, false, false, true, renderArgs->_renderMethod == render::Args::FORWARD);
                geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, laserLength, 0.0f), laserColor, _rightPointerGeometryID);
            }
        }
    }

    ViewFrustum frustum = renderArgs->getViewFrustum();
    if (!frustum.sphereIntersectsFrustum(getWorldPosition(), getBoundingRadius())) {
        return;
    }

    if (showCollisionShapes && shouldRenderHead(renderArgs) && _skeletonModel->isRenderable()) {
        PROFILE_RANGE_BATCH(batch, __FUNCTION__":skeletonBoundingCollisionShapes");
        const float BOUNDING_SHAPE_ALPHA = 0.7f;
        _skeletonModel->renderBoundingCollisionShapes(renderArgs, *renderArgs->_batch, getModelScale(), BOUNDING_SHAPE_ALPHA);
    }

    if (showReceiveStats || showNamesAboveHeads) {
        glm::vec3 toTarget = frustum.getPosition() - getWorldPosition();
        float distanceToTarget = glm::length(toTarget);
        const float DISPLAYNAME_DISTANCE = 20.0f;
        updateDisplayNameAlpha(distanceToTarget < DISPLAYNAME_DISTANCE);
        if (!isMyAvatar() || !(renderArgs->_cameraMode == (int8_t)CAMERA_MODE_FIRST_PERSON_LOOK_AT
                          || renderArgs->_cameraMode == (int8_t)CAMERA_MODE_FIRST_PERSON)) {
            auto& frustum = renderArgs->getViewFrustum();
            auto textPosition = getDisplayNamePosition();
            if (frustum.pointIntersectsFrustum(textPosition)) {
                renderDisplayName(batch, frustum, textPosition, renderArgs->_renderMethod == render::Args::FORWARD);
            }
        }
    }
}

void Avatar::setEnableMeshVisible(bool isEnabled) {
    if (_isMeshVisible != isEnabled) {
        _isMeshVisible = isEnabled;
        _needMeshVisibleSwitch = true;
    }
}

bool Avatar::getEnableMeshVisible() const {
    return _isMeshVisible;
}

void Avatar::fixupModelsInScene(const render::ScenePointer& scene) {
    bool canTryFade{ false };

    _attachmentsToDelete.clear();

    // check to see if when we added our models to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::Transaction transaction;
    if (_skeletonModel->isRenderable() && _skeletonModel->needsFixupInScene()) {
        _skeletonModel->removeFromScene(scene, transaction);
        using namespace std::placeholders;
        _skeletonModel->addToScene(scene, transaction, std::bind(&Avatar::metaBlendshapeOperator, _renderItemID, _1, _2, _3, _4));

        _skeletonModel->setTagMask(render::hifi::TAG_ALL_VIEWS);
        _skeletonModel->setGroupCulled(true);
        _skeletonModel->setCanCastShadow(true);
        _skeletonModel->setVisibleInScene(_isMeshVisible, scene);

        processMaterials();
        canTryFade = true;
        _isAnimatingScale = true;
    }
    bool attachmentRenderingNeedsUpdate = false;
    for (auto attachmentModel : _attachmentModels) {
        if (attachmentModel->isRenderable() && attachmentModel->needsFixupInScene()) {
            attachmentModel->removeFromScene(scene, transaction);
            attachmentModel->addToScene(scene, transaction);

            attachmentModel->setTagMask(render::hifi::TAG_ALL_VIEWS);
            attachmentModel->setGroupCulled(true);
            attachmentModel->setCanCastShadow(true);
            attachmentModel->setVisibleInScene(_isMeshVisible, scene);
            attachmentRenderingNeedsUpdate = true;
        }
    }

    if (_needMeshVisibleSwitch) {
        _skeletonModel->setVisibleInScene(_isMeshVisible, scene);
        for (auto attachmentModel : _attachmentModels) {
            if (attachmentModel->isRenderable()) {
                attachmentModel->setVisibleInScene(_isMeshVisible, scene);
            }
        }
        updateRenderItem(transaction);
        _needMeshVisibleSwitch = false;
    }

    if (_mustFadeIn && canTryFade) {
        // Do it now to be sure all the sub items are ready and the fade is sent to them too
        fade(transaction, render::Transition::USER_ENTER_DOMAIN);
        _mustFadeIn = false;
    }

    for (auto attachmentModelToRemove : _attachmentsToRemove) {
        attachmentModelToRemove->removeFromScene(scene, transaction);
        attachmentRenderingNeedsUpdate = true;
    }
    _attachmentsToDelete.insert(_attachmentsToDelete.end(), _attachmentsToRemove.begin(), _attachmentsToRemove.end());
    _attachmentsToRemove.clear();

    if (attachmentRenderingNeedsUpdate) {
        updateAttachmentRenderIDs();
    }

    scene->enqueueTransaction(transaction);
}

bool Avatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    return true;
}

void Avatar::simulateAttachments(float deltaTime) {
    assert(_attachmentModels.size() == _attachmentModelsTexturesLoaded.size());
    PerformanceTimer perfTimer("attachments");
    for (int i = 0; i < (int)_attachmentModels.size(); i++) {
        const AttachmentData& attachment = _attachmentData.at(i);
        auto& model = _attachmentModels.at(i);
        bool texturesLoaded = _attachmentModelsTexturesLoaded.at(i);

        // Watch for texture loading
        if (!texturesLoaded && model->getGeometry() && model->getGeometry()->areTexturesLoaded()) {
            _attachmentModelsTexturesLoaded[i] = true;
            model->updateRenderItems();
        }

        int jointIndex = getJointIndex(attachment.jointName);
        glm::vec3 jointPosition;
        glm::quat jointRotation;
        if (attachment.isSoft) {
            // soft attachments do not have transform offsets
            model->setTransformNoUpdateRenderItems(Transform(getWorldOrientation() * Quaternions::Y_180, glm::vec3(1.0), getWorldPosition()));
            model->simulate(deltaTime);
            model->updateRenderItems();
        } else {
            if (_skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPosition) &&
                _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRotation)) {
                model->setTransformNoUpdateRenderItems(Transform(jointRotation * attachment.rotation, glm::vec3(1.0), jointPosition + jointRotation * attachment.translation * getModelScale()));
                float scale = getModelScale() * attachment.scale;
                model->setScaleToFit(true, model->getNaturalDimensions() * scale, true); // hack to force rescale
                model->setSnapModelToCenter(false); // hack to force resnap
                model->setSnapModelToCenter(true);
                model->simulate(deltaTime);
                model->updateRenderItems();
            }
        }
    }

    if (_ancestorChainRenderableVersion != _lastAncestorChainRenderableVersion) {
        _lastAncestorChainRenderableVersion = _ancestorChainRenderableVersion;
        updateDescendantRenderIDs();
    }
}

float Avatar::getBoundingRadius() const {
    return getBounds().getLargestDimension() / 2.0f;
}

#ifdef DEBUG
void debugValue(const QString& str, const glm::vec3& value) {
    if (glm::any(glm::isnan(value)) || glm::any(glm::isinf(value))) {
        qCWarning(avatars_renderer) << "debugValue() " << str << value;
    }
};
void debugValue(const QString& str, const float& value) {
    if (glm::isnan(value) || glm::isinf(value)) {
        qCWarning(avatars_renderer) << "debugValue() " << str << value;
   }
};
#define DEBUG_VALUE(str, value) debugValue(str, value)
#else
#define DEBUG_VALUE(str, value)
#endif

glm::vec3 Avatar::getDisplayNamePosition() const {
    glm::vec3 namePosition(0.0f);
    glm::vec3 bodyUpDirection = getBodyUpDirection();
    DEBUG_VALUE("bodyUpDirection =", bodyUpDirection);

    if (_skeletonModel->getNeckPosition(namePosition)) {
        float headHeight = getHeadHeight();
        DEBUG_VALUE("namePosition =", namePosition);
        DEBUG_VALUE("headHeight =", headHeight);

        static const float SLIGHTLY_ABOVE = 1.1f;
        namePosition += bodyUpDirection * headHeight * SLIGHTLY_ABOVE;
    } else {
        const float HEAD_PROPORTION = 0.75f;
        float size = getBoundingRadius();

        DEBUG_VALUE("_position =", getWorldPosition());
        DEBUG_VALUE("size =", size);
        namePosition = getWorldPosition() + bodyUpDirection * (size * HEAD_PROPORTION);
    }

    if (glm::any(glm::isnan(namePosition)) || glm::any(glm::isinf(namePosition))) {
        qCWarning(avatars_renderer) << "Invalid display name position" << namePosition
                                << ", setting is to (0.0f, 0.5f, 0.0f)";
        namePosition = glm::vec3(0.0f, 0.5f, 0.0f);
    }

    return namePosition;
}

Transform Avatar::calculateDisplayNameTransform(const ViewFrustum& view, const glm::vec3& textPosition) const {
    Q_ASSERT_X(view.pointIntersectsFrustum(textPosition),
               "Avatar::calculateDisplayNameTransform", "Text not in viewfrustum.");
    glm::vec3 toFrustum = view.getPosition() - textPosition;

    // Compute orientation
    // If x and z are 0, atan(x, z) adais undefined, so default to 0 degrees
    const float yawRotation = (toFrustum.x == 0.0f && toFrustum.z == 0.0f) ? 0.0f : glm::atan(toFrustum.x, toFrustum.z);
    glm::quat orientation = glm::quat(glm::vec3(0.0f, yawRotation, 0.0f));

    // Compute correct scale to apply
    static const float DESIRED_HEIGHT_RAD = glm::radians(1.5f);
    float scale = glm::length(toFrustum) * glm::tan(DESIRED_HEIGHT_RAD);

    // Set transform
    Transform result;
    result.setTranslation(textPosition);
    result.setRotation(orientation); // Always face the screen
    result.setScale(scale);
    // raise by half the scale up so that textPosition be the bottom
    result.postTranslate(Vectors::UP / 2.0f);

    return result;
}

void Avatar::renderDisplayName(gpu::Batch& batch, const ViewFrustum& view, const glm::vec3& textPosition, bool forward) const {
    PROFILE_RANGE_BATCH(batch, __FUNCTION__);

    bool shouldShowReceiveStats = showReceiveStats && !isMyAvatar();

    // If we have nothing to draw, or it's totally transparent, or it's too close or behind the camera, return
    static const float CLIP_DISTANCE = 0.2f;
    if ((_displayName.isEmpty() && !shouldShowReceiveStats) || _displayNameAlpha == 0.0f
        || (glm::dot(view.getDirection(), getDisplayNamePosition() - view.getPosition()) <= CLIP_DISTANCE)) {
        return;
    }

    // optionally render timing stats for this avatar with the display name
    QString renderedDisplayName = _displayName;
    if (shouldShowReceiveStats) {
        float kilobitsPerSecond = getAverageBytesReceivedPerSecond() / (float) BYTES_PER_KILOBIT;

        QString statsFormat = QString("(%1 Kbps, %2 Hz)");
        if (!renderedDisplayName.isEmpty()) {
            statsFormat.prepend(" - ");
        }
        renderedDisplayName += statsFormat.arg(QString::number(kilobitsPerSecond, 'f', 2)).arg(getReceiveRate());
    }

    // Compute display name extent/position offset
    static TextRenderer3D* displayNameRenderer = TextRenderer3D::getInstance(ROBOTO_FONT_FAMILY);
    const glm::vec2 extent = displayNameRenderer->computeExtent(renderedDisplayName);
    if (!glm::any(glm::isCompNull(extent, EPSILON))) {
        const QRect nameDynamicRect = QRect(0, 0, (int)extent.x, (int)extent.y);
        const int text_x = -nameDynamicRect.width() / 2;
        const int text_y = -nameDynamicRect.height() / 2;

        // Compute background position/size
        static const float SLIGHTLY_IN_FRONT = 0.1f;
        static const float BORDER_RELATIVE_SIZE = 0.1f;
        static const float BEVEL_FACTOR = 0.1f;
        const int border = BORDER_RELATIVE_SIZE * nameDynamicRect.height();
        const int left = text_x - border;
        const int bottom = text_y - border;
        const int width = nameDynamicRect.width() + 2.0f * border;
        const int height = nameDynamicRect.height() + 2.0f * border;
        const int bevelDistance = BEVEL_FACTOR * height;

        // Display name and background colors
        glm::vec4 textColor(0.93f, 0.93f, 0.93f, _displayNameAlpha);
        glm::vec4 backgroundColor(0.2f, 0.2f, 0.2f,
                                  (_displayNameAlpha / DISPLAYNAME_ALPHA) * DISPLAYNAME_BACKGROUND_ALPHA);

        // Compute display name transform
        auto textTransform = calculateDisplayNameTransform(view, textPosition);
        // Test on extent above insures abs(height) > 0.0f
        textTransform.postScale(1.0f / height);
        batch.setModelTransform(textTransform);

        {
            PROFILE_RANGE_BATCH(batch, __FUNCTION__":renderBevelCornersRect");
            DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch, false, false, true, true, true, forward);
            DependencyManager::get<GeometryCache>()->renderBevelCornersRect(batch, left, bottom, width, height,
                bevelDistance, backgroundColor, _nameRectGeometryID);
        }

        // Render actual name
        QByteArray nameUTF8 = renderedDisplayName.toLocal8Bit();

        // Render text slightly in front to avoid z-fighting
        textTransform.postTranslate(glm::vec3(0.0f, 0.0f, SLIGHTLY_IN_FRONT * displayNameRenderer->getFontSize()));
        batch.setModelTransform(textTransform);
        {
            PROFILE_RANGE_BATCH(batch, __FUNCTION__":renderText");
            displayNameRenderer->draw(batch, text_x, -text_y, glm::vec2(-1.0f), nameUTF8.data(), textColor, true, forward);
        }
    }
}

void Avatar::setSkeletonOffset(const glm::vec3& offset) {
    const float MAX_OFFSET_LENGTH = getModelScale() * 0.5f;
    float offsetLength = glm::length(offset);
    if (offsetLength > MAX_OFFSET_LENGTH) {
        _skeletonOffset = (MAX_OFFSET_LENGTH / offsetLength) * offset;
    } else {
        _skeletonOffset = offset;
    }
}

glm::vec3 Avatar::getSkeletonPosition() const {
    // The avatar is rotated PI about the yAxis, so we have to correct for it
    // to get the skeleton offset contribution in the world-frame.
    const glm::quat FLIP = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
    return getWorldPosition() + getWorldOrientation() * FLIP * _skeletonOffset;
}

QVector<glm::quat> Avatar::getJointRotations() const {
    if (QThread::currentThread() != thread()) {
        return AvatarData::getJointRotations();
    }
    QVector<glm::quat> jointRotations(_skeletonModel->getJointStateCount());
    for (int i = 0; i < _skeletonModel->getJointStateCount(); ++i) {
        _skeletonModel->getJointRotation(i, jointRotations[i]);
    }
    return jointRotations;
}

QVector<glm::vec3> Avatar::getJointTranslations() const {
    if (QThread::currentThread() != thread()) {
        return AvatarData::getJointTranslations();
    }
    QVector<glm::vec3> jointTranslations(_skeletonModel->getJointStateCount());
    for (int i = 0; i < _skeletonModel->getJointStateCount(); ++i) {
        _skeletonModel->getJointTranslation(i, jointTranslations[i]);
    }
    return jointTranslations;
}

glm::quat Avatar::getJointRotation(int index) const {
    glm::quat rotation;
    _skeletonModel->getJointRotation(index, rotation);
    return rotation;
}

glm::vec3 Avatar::getJointTranslation(int index) const {
    glm::vec3 translation;
    _skeletonModel->getJointTranslation(index, translation);
    return translation;
}

glm::quat Avatar::getDefaultJointRotation(int index) const {
    glm::quat rotation;
    _skeletonModel->getRelativeDefaultJointRotation(index, rotation);
    return rotation;
}

glm::vec3 Avatar::getDefaultJointTranslation(int index) const {
    glm::vec3 translation;
    _skeletonModel->getRelativeDefaultJointTranslation(index, translation);
    return translation;
}

glm::quat Avatar::getAbsoluteDefaultJointRotationInObjectFrame(int index) const {
    // To make this thread safe, we hold onto the model by smart ptr, which prevents it from being deleted while we are accessing it.
    auto model = getSkeletonModel();
    if (model) {
        auto skeleton = model->getRig().getAnimSkeleton();
        if (skeleton && index >= 0 && index < skeleton->getNumJoints()) {
            // The rotation part of the geometry-to-rig transform is always identity so we can skip it.
            // Y_180 is to convert from rig-frame into avatar-frame
            return Quaternions::Y_180 * skeleton->getAbsoluteDefaultPose(index).rot();
        }
    }
    return Quaternions::Y_180;
}

glm::vec3 Avatar::getAbsoluteDefaultJointTranslationInObjectFrame(int index) const {
    // To make this thread safe, we hold onto the model by smart ptr, which prevents it from being deleted while we are accessing it.
    auto model = getSkeletonModel();
    if (model) {
        const Rig& rig = model->getRig();
        auto skeleton = rig.getAnimSkeleton();
        if (skeleton && index >= 0 && index < skeleton->getNumJoints()) {
            // trans is in geometry frame.
            glm::vec3 trans = skeleton->getAbsoluteDefaultPose(index).trans();
            // Y_180 is to convert from rig-frame into avatar-frame
            glm::mat4 geomToAvatarMat = Matrices::Y_180 * rig.getGeometryToRigTransform();
            return transformPoint(geomToAvatarMat, trans);
        }
    }
    return Vectors::ZERO;
}

glm::quat Avatar::getAbsoluteJointRotationInObjectFrame(int index) const {
    if (index < 0) {
        index += numeric_limits<unsigned short>::max() + 1; // 65536
    }

    switch (index) {
        case NO_JOINT_INDEX: {
            return glm::quat();
        }
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            glm::mat4 avatarMatrix = getLocalTransform().getMatrix();
            glm::mat4 finalMat = glm::inverse(avatarMatrix) * sensorToWorldMatrix;
            return glmExtractRotation(finalMat);
        }
        case CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX:
        case CONTROLLER_LEFTHAND_INDEX: {
            Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
            return controllerLeftHandTransform.getRotation();
        }
        case CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX:
        case CONTROLLER_RIGHTHAND_INDEX: {
            Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
            return controllerRightHandTransform.getRotation();
        }
        case CAMERA_MATRIX_INDEX: {
            glm::quat rotation;
            if (_skeletonModel && _skeletonModel->isLoaded()) {
                int headJointIndex = getJointIndex("Head");
                if (headJointIndex >= 0) {
                    _skeletonModel->getAbsoluteJointRotationInRigFrame(headJointIndex, rotation);
                }
            }
            return Quaternions::Y_180 * rotation * Quaternions::Y_180;
        }
        case FARGRAB_RIGHTHAND_INDEX: {
            return extractRotation(_farGrabRightMatrixCache.get());
        }
        case FARGRAB_LEFTHAND_INDEX: {
            return extractRotation(_farGrabLeftMatrixCache.get());
        }
        case FARGRAB_MOUSE_INDEX: {
            return extractRotation(_farGrabMouseMatrixCache.get());
        }
        default: {
            glm::quat rotation;
            _skeletonModel->getAbsoluteJointRotationInRigFrame(index, rotation);
            return Quaternions::Y_180 * rotation;
        }
    }
}

glm::vec3 Avatar::getAbsoluteJointTranslationInObjectFrame(int index) const {
    if (index < 0) {
        index += numeric_limits<unsigned short>::max() + 1; // 65536
    }

    switch (index) {
        case NO_JOINT_INDEX: {
            return glm::vec3(0.0f);
        }
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            glm::mat4 avatarMatrix = getLocalTransform().getMatrix();
            glm::mat4 finalMat = glm::inverse(avatarMatrix) * sensorToWorldMatrix;
            return extractTranslation(finalMat);
        }
        case CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX:
        case CONTROLLER_LEFTHAND_INDEX: {
            Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
            return controllerLeftHandTransform.getTranslation();
        }
        case CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX:
        case CONTROLLER_RIGHTHAND_INDEX: {
            Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
            return controllerRightHandTransform.getTranslation();
        }
        case CAMERA_MATRIX_INDEX: {
            glm::vec3 translation;
            if (_skeletonModel && _skeletonModel->isLoaded()) {
                int headJointIndex = getJointIndex("Head");
                if (headJointIndex >= 0) {
                    _skeletonModel->getAbsoluteJointTranslationInRigFrame(headJointIndex, translation);
                }
            }
            return Quaternions::Y_180 * translation * Quaternions::Y_180;
        }
        case FARGRAB_RIGHTHAND_INDEX: {
            return extractTranslation(_farGrabRightMatrixCache.get());
        }
        case FARGRAB_LEFTHAND_INDEX: {
            return extractTranslation(_farGrabLeftMatrixCache.get());
        }
        case FARGRAB_MOUSE_INDEX: {
            return extractTranslation(_farGrabMouseMatrixCache.get());
        }
        default: {
            glm::vec3 translation;
            _skeletonModel->getAbsoluteJointTranslationInRigFrame(index, translation);
            return Quaternions::Y_180 * translation;
        }
    }
}

glm::vec3 Avatar::getAbsoluteJointScaleInObjectFrame(int index) const {
    if (index < 0) {
        index += numeric_limits<unsigned short>::max() + 1; // 65536
    }

    // only sensor to world matrix has non identity scale.
    switch (index) {
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            return extractScale(sensorToWorldMatrix);
        }
        default:
            return AvatarData::getAbsoluteJointScaleInObjectFrame(index);
    }
}


glm::vec3 Avatar::worldToJointPoint(const glm::vec3& position, const int jointIndex) const {
    glm::vec3 jointPos = getWorldPosition();//default value if no or invalid joint specified
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if (jointIndex != -1) {
        if (_skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPos)) {
            _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot);
        } else {
            qWarning() << "Invalid joint index specified: " << jointIndex;
        }
    }
    glm::vec3 modelOffset = position - jointPos;
    glm::vec3 jointSpacePosition = glm::inverse(jointRot) * modelOffset;

    return jointSpacePosition;
}

glm::vec3 Avatar::worldToJointDirection(const glm::vec3& worldDir, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }

    glm::vec3 jointSpaceDir = glm::inverse(jointRot) * worldDir;
    return jointSpaceDir;
}

glm::quat Avatar::worldToJointRotation(const glm::quat& worldRot, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::quat jointSpaceRot = glm::inverse(jointRot) * worldRot;
    return jointSpaceRot;
}

glm::vec3 Avatar::jointToWorldPoint(const glm::vec3& jointSpacePos, const int jointIndex) const {
    glm::vec3 jointPos = getWorldPosition();//default value if no or invalid joint specified
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified

    if (jointIndex != -1) {
        if (_skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPos)) {
            _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot);
        } else {
            qWarning() << "Invalid joint index specified: " << jointIndex;
        }
    }

    glm::vec3 worldOffset = jointRot * jointSpacePos;
    glm::vec3 worldPos = jointPos + worldOffset;

    return worldPos;
}

glm::vec3 Avatar::jointToWorldDirection(const glm::vec3& jointSpaceDir, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::vec3 worldDir = jointRot * jointSpaceDir;
    return worldDir;
}

glm::quat Avatar::jointToWorldRotation(const glm::quat& jointSpaceRot, const int jointIndex) const {
    glm::quat jointRot = getWorldOrientation();//default value if no or invalid joint specified
    if ((jointIndex != -1) && (!_skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRot))) {
        qWarning() << "Invalid joint index specified: " << jointIndex;
    }
    glm::quat worldRot = jointRot * jointSpaceRot;
    return worldRot;
}


void Avatar::invalidateJointIndicesCache() const {
    QWriteLocker writeLock(&_modelJointIndicesCacheLock);
    _modelJointsCached = false;
}

void Avatar::withValidJointIndicesCache(std::function<void()> const& worker) const {
    QReadLocker readLock(&_modelJointIndicesCacheLock);
    if (_modelJointsCached) {
        worker();
    } else {
        readLock.unlock();
        {
            QWriteLocker writeLock(&_modelJointIndicesCacheLock);
            if (!_modelJointsCached) {
                _modelJointIndicesCache.clear();
                if (_skeletonModel && _skeletonModel->isLoaded()) {
                    _modelJointIndicesCache = _skeletonModel->getHFMModel().jointIndices;
                    _modelJointsCached = true;
                }
            }
            worker();
        }
    }
}

int Avatar::getJointIndex(const QString& name) const {
    int result = getFauxJointIndex(name);
    if (result != -1) {
        return result;
    }

    withValidJointIndicesCache([&]() {
        result = _modelJointIndicesCache.value(name, result + 1) - 1;
    });
    return result;
}

QStringList Avatar::getJointNames() const {
    QStringList result;
    withValidJointIndicesCache([&]() {
        // find out how large the vector needs to be
        int maxJointIndex = -1;
        for (auto k = _modelJointIndicesCache.constBegin(); k != _modelJointIndicesCache.constEnd(); k++) {
            int index = k.value();
            if (index > maxJointIndex) {
                maxJointIndex = index;
            }
        }
        // iterate through the hash and put joint names
        // into the vector at their indices
        QVector<QString> resultVector(maxJointIndex+1);
        for (auto i = _modelJointIndicesCache.constBegin(); i != _modelJointIndicesCache.constEnd(); i++) {
            int index = i.value();
            resultVector[index] = i.key();
        }
        // convert to QList and drop out blanks
        result = resultVector.toList();
        QMutableListIterator<QString> j(result);
        while (j.hasNext()) {
            QString jointName = j.next();
            if (jointName.isEmpty()) {
                j.remove();
            }
        }
    });
    return result;
}

std::vector<AvatarSkeletonTrait::UnpackedJointData> Avatar::getSkeletonDefaultData() {
    std::vector<AvatarSkeletonTrait::UnpackedJointData> defaultSkeletonData;
    if (_skeletonModel->isLoaded()) {
        auto& model = _skeletonModel->getHFMModel();
        auto& rig = _skeletonModel->getRig();
        float geometryToRigScale = extractScale(rig.getGeometryToRigTransform())[0];
        QStringList jointNames = getJointNames();
        int sizeCount = 0;
        for (int i = 0; i < jointNames.size(); i++) {
            AvatarSkeletonTrait::UnpackedJointData jointData;
            jointData.jointIndex = i;
            jointData.parentIndex = rig.getJointParentIndex(i);
            if (jointData.parentIndex == -1) {
                jointData.boneType = model.joints[i].isSkeletonJoint ? AvatarSkeletonTrait::BoneType::SkeletonRoot : AvatarSkeletonTrait::BoneType::NonSkeletonRoot;
            } else {
                jointData.boneType = model.joints[i].isSkeletonJoint ? AvatarSkeletonTrait::BoneType::SkeletonChild : AvatarSkeletonTrait::BoneType::NonSkeletonChild;
            }
            jointData.defaultRotation = rig.getAbsoluteDefaultPose(i).rot();
            jointData.defaultTranslation = getDefaultJointTranslation(i);
            float jointLocalScale = extractScale(model.joints[i].transform)[0];
            jointData.defaultScale = jointLocalScale / geometryToRigScale;
            jointData.jointName = jointNames[i];
            jointData.stringLength = jointNames[i].size();
            jointData.stringStart = sizeCount;
            sizeCount += jointNames[i].size();
            defaultSkeletonData.push_back(jointData);
        }
    }
    return defaultSkeletonData;
}

glm::vec3 Avatar::getJointPosition(int index) const {
    glm::vec3 position;
    _skeletonModel->getJointPositionInWorldFrame(index, position);
    return position;
}

glm::vec3 Avatar::getJointPosition(const QString& name) const {
    glm::vec3 position;
    _skeletonModel->getJointPositionInWorldFrame(getJointIndex(name), position);
    return position;
}

void Avatar::scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const {
    //Scale a world space vector as if it was relative to the position
    positionToScale = getWorldPosition() + getModelScale() * (positionToScale - getWorldPosition());
}

void Avatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSkeletonModelURL", Q_ARG(const QUrl&, skeletonModelURL));
        return;
    }

    AvatarData::setSkeletonModelURL(skeletonModelURL);

    if (!isMyAvatar() && !DependencyManager::get<NodeList>()->isIgnoringNode(getSessionUUID())) {
        createOrb();
    }
    indicateLoadingStatus(LoadingStatus::LoadModel);

    _skeletonModel->setURL(getSkeletonModelURL());
}

void Avatar::setModelURLFinished(bool success) {
    invalidateJointIndicesCache();

    _isAnimatingScale = true;
    _reconstructSoftEntitiesJointMap = true;

    if (!success && _skeletonModelURL != AvatarData::defaultFullAvatarModelUrl()) {
        indicateLoadingStatus(LoadingStatus::LoadFailure);
        const int MAX_SKELETON_DOWNLOAD_ATTEMPTS = 4; // NOTE: we don't want to be as generous as ResourceCache is, we only want 4 attempts
        if (_skeletonModel->getResourceDownloadAttemptsRemaining() <= 0 ||
            _skeletonModel->getResourceDownloadAttempts() > MAX_SKELETON_DOWNLOAD_ATTEMPTS) {
            qCWarning(avatars_renderer) << "Using default after failing to load Avatar model, "
                << "after" << _skeletonModel->getResourceDownloadAttempts() << "attempts.";

            // call _skeletonModel.setURL, but leave our copy of _skeletonModelURL alone.  This is so that
            // we don't redo this every time we receive an identity packet from the avatar with the bad url.
            QMetaObject::invokeMethod(_skeletonModel.get(), "setURL",
                Qt::QueuedConnection, Q_ARG(QUrl, AvatarData::defaultFullAvatarModelUrl()));
        } else {
            qCWarning(avatars_renderer) << "Avatar model failed to load... attempts:"
                << _skeletonModel->getResourceDownloadAttempts() << "out of:" << MAX_SKELETON_DOWNLOAD_ATTEMPTS;
        }
    }
    if (success) {
        indicateLoadingStatus(LoadingStatus::LoadSuccess);
    }
}

// rig is ready
void Avatar::rigReady() {
    buildUnscaledEyeHeightCache();
    buildSpine2SplineRatioCache();
    computeMultiSphereShapes();
    buildSpine2SplineRatioCache();
    setSkeletonData(getSkeletonDefaultData());
    sendSkeletonData();
}

// rig has been reset.
void Avatar::rigReset() {
    clearUnscaledEyeHeightCache();
    clearSpine2SplineRatioCache();
}

void Avatar::computeMultiSphereShapes() {
    const Rig& rig = getSkeletonModel()->getRig();
    const HFMModel& geometry = getSkeletonModel()->getHFMModel();
    glm::vec3 geometryScale = extractScale(rig.getGeometryOffsetPose());
    int jointCount = rig.getJointStateCount();
    _multiSphereShapes.clear();
    _multiSphereShapes.reserve(jointCount);
    for (int i = 0; i < jointCount; i++) {
        const HFMJointShapeInfo& shapeInfo = geometry.joints[i].shapeInfo;
        std::vector<btVector3> btPoints;
        int lineCount = (int)shapeInfo.debugLines.size();
        btPoints.reserve(lineCount);
        glm::vec3 jointScale = rig.getJointPose(i).scale() / extractScale(rig.getGeometryToRigTransform());
        for (int j = 0; j < lineCount; j++) {
            const glm::vec3 &point = shapeInfo.debugLines[j];
            auto rigPoint = jointScale * geometryScale * point;
            btVector3 btPoint = glmToBullet(rigPoint);
            btPoints.push_back(btPoint);
        }
        auto jointName = rig.nameOfJoint(i).toUpper();
        MultiSphereShape multiSphereShape;
        if (multiSphereShape.computeMultiSphereShape(i, jointName, btPoints)) {
            multiSphereShape.calculateDebugLines();
            multiSphereShape.setScale(_targetScale);
        }
        _multiSphereShapes.push_back(multiSphereShape);
    }
}

void Avatar::updateFitBoundingBox() {
    _fitBoundingBox = AABox();
    if (getJointCount() == (int)_multiSphereShapes.size()) {
        for (int i = 0; i < getJointCount(); i++) {
            auto &shape = _multiSphereShapes[i];
            glm::vec3 jointPosition;
            glm::quat jointRotation;
            _skeletonModel->getJointPositionInWorldFrame(i, jointPosition);
            _skeletonModel->getJointRotationInWorldFrame(i, jointRotation);
            _fitBoundingBox += shape.updateBoundingBox(jointPosition, jointRotation);
        }
    }
}

// create new model, can return an instance of a SoftAttachmentModel rather then Model
static std::shared_ptr<Model> allocateAttachmentModel(bool isSoft, const Rig& rigOverride, bool isCauterized) {
    if (isSoft) {
        // cast to std::shared_ptr<Model>
        std::shared_ptr<SoftAttachmentModel> softModel = std::make_shared<SoftAttachmentModel>(nullptr, rigOverride);
        if (isCauterized) {
            softModel->flagAsCauterized();
        }
        return std::dynamic_pointer_cast<Model>(softModel);
    } else {
        return std::make_shared<Model>();
    }
}

void Avatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "setAttachmentData",
                                  Q_ARG(const QVector<AttachmentData>, attachmentData));
        return;
    }

    auto oldAttachmentData = _attachmentData;
    AvatarData::setAttachmentData(attachmentData);

    // if number of attachments has been reduced, remove excess models.
    while ((int)_attachmentModels.size() > attachmentData.size()) {
        auto attachmentModel = _attachmentModels.back();
        _attachmentModels.pop_back();
        _attachmentModelsTexturesLoaded.pop_back();
        _attachmentsToRemove.push_back(attachmentModel);
    }

    for (int i = 0; i < attachmentData.size(); i++) {
        if (i == (int)_attachmentModels.size()) {
            // if number of attachments has been increased, we need to allocate a new model
            _attachmentModels.push_back(allocateAttachmentModel(attachmentData[i].isSoft, _skeletonModel->getRig(), isMyAvatar()));
            _attachmentModelsTexturesLoaded.push_back(false);
        } else if (i < oldAttachmentData.size() && oldAttachmentData[i].isSoft != attachmentData[i].isSoft) {
            // if the attachment has changed type, we need to re-allocate a new one.
            _attachmentsToRemove.push_back(_attachmentModels[i]);
            _attachmentModels[i] = allocateAttachmentModel(attachmentData[i].isSoft, _skeletonModel->getRig(), isMyAvatar());
            _attachmentModelsTexturesLoaded[i] = false;
        }
        // If the model URL has changd, we need to wait for the textures to load
        if (_attachmentModels[i]->getURL() != attachmentData[i].modelURL) {
            _attachmentModelsTexturesLoaded[i] = false;
        }
        _attachmentModels[i]->setLoadingPriority(ATTACHMENT_LOADING_PRIORITY);
        _attachmentModels[i]->setURL(attachmentData[i].modelURL);
    }
}

int Avatar::parseDataFromBuffer(const QByteArray& buffer) {
    PerformanceTimer perfTimer("unpack");
    if (!_initialized) {
        // now that we have data for this Avatar we are go for init
        init();
    }

    // change in position implies movement
    glm::vec3 oldPosition = getWorldPosition();

    int bytesRead = AvatarData::parseDataFromBuffer(buffer);

    const float MOVE_DISTANCE_THRESHOLD = 0.001f;
    _moving = glm::distance(oldPosition, getWorldPosition()) > MOVE_DISTANCE_THRESHOLD;
    if (_moving || _hasNewJointData) {
        locationChanged();
    }

    return bytesRead;
}

float Avatar::getSkeletonHeight() const {
    Extents extents = _skeletonModel->getBindExtents();
    return extents.maximum.y - extents.minimum.y;
}

float Avatar::getHeadHeight() const {
    Extents extents = _skeletonModel->getMeshExtents();
    glm::vec3 neckPosition;
    if (!extents.isEmpty() && extents.isValid() && _skeletonModel->getNeckPosition(neckPosition)) {
        return extents.maximum.y / 2.0f - neckPosition.y + getWorldPosition().y;
    }

    const float DEFAULT_HEAD_HEIGHT = 0.25f;
    return DEFAULT_HEAD_HEIGHT;
}

float Avatar::getPelvisFloatingHeight() const {
    return -_skeletonModel->getBindExtents().minimum.y;
}

void Avatar::updateDisplayNameAlpha(bool showDisplayName) {
    if (!(showNamesAboveHeads || showReceiveStats)) {
        _displayNameAlpha = 0.0f;
        return;
    }

    // For myAvatar, the alpha update is not done (called in simulate for other avatars)
    if (isMyAvatar()) {
        if (showDisplayName) {
            _displayNameAlpha = DISPLAYNAME_ALPHA;
        } else {
            _displayNameAlpha = 0.0f;
        }
    }

    if (showDisplayName) {
        _displayNameTargetAlpha = DISPLAYNAME_ALPHA;
    } else {
        _displayNameTargetAlpha = 0.0f;
    }
}

void Avatar::computeShapeInfo(ShapeInfo& shapeInfo) {
    float uniformScale = getModelScale();
    float radius = glm::max(MIN_AVATAR_RADIUS, uniformScale * _skeletonModel->getBoundingCapsuleRadius());
    float height = glm::max(MIN_AVATAR_HEIGHT, uniformScale *  _skeletonModel->getBoundingCapsuleHeight());
    shapeInfo.setCapsuleY(radius, 0.5f * height);
    glm::vec3 offset = uniformScale * _skeletonModel->getBoundingCapsuleOffset();
    shapeInfo.setOffset(offset);
}

void Avatar::computeDetailedShapeInfo(ShapeInfo& shapeInfo, int jointIndex) {
    if (jointIndex > -1 && jointIndex < (int)_multiSphereShapes.size()) {
        auto& data = _multiSphereShapes[jointIndex].getSpheresData();
        if (data.size() > 0) {
            std::vector<glm::vec3> positions;
            std::vector<btScalar> radiuses;
            positions.reserve(data.size());
            radiuses.reserve(data.size());
            for (auto& sphere : data) {
                positions.push_back(sphere._position);
                radiuses.push_back(sphere._radius);
            }
            shapeInfo.setMultiSphere(positions, radiuses);
        }
    }
}

void Avatar::getCapsule(glm::vec3& start, glm::vec3& end, float& radius) {
    ShapeInfo shapeInfo;
    computeShapeInfo(shapeInfo);
    glm::vec3 halfExtents = shapeInfo.getHalfExtents(); // x = radius, y = cylinderHalfHeight + radius
    radius = halfExtents.x;
    glm::vec3 halfCylinderAxis(0.0f, halfExtents.y - radius, 0.0f);
    Transform transform = getTransform();
    start = transform.getTranslation() + transform.getRotation() * (shapeInfo.getOffset() - halfCylinderAxis);
    end = transform.getTranslation() + transform.getRotation() * (shapeInfo.getOffset() + halfCylinderAxis);
}

glm::vec3 Avatar::getWorldFeetPosition() {
    ShapeInfo shapeInfo;
    computeShapeInfo(shapeInfo);
    glm::vec3 halfExtents = shapeInfo.getHalfExtents(); // x = radius, y = cylinderHalfHeight + radius
    glm::vec3 localFeet(0.0f, shapeInfo.getOffset().y - halfExtents.y, 0.0f);
    Transform transform = getTransform();
    return transform.getTranslation() + transform.getRotation() * localFeet;
}

float Avatar::computeMass() {
    float radius;
    glm::vec3 start, end;
    getCapsule(start, end, radius);
    // NOTE:
    // volumeOfCapsule = volumeOfCylinder + volumeOfSphere
    // volumeOfCapsule = (2PI * R^2 * H) + (4PI * R^3 / 3)
    // volumeOfCapsule = 2PI * R^2 * (H + 2R/3)
    return _density * TWO_PI * radius * radius * (glm::length(end - start) + 2.0f * radius / 3.0f);
}

// thread-safe
glm::vec3 Avatar::getLeftPalmPosition() const {
    return _leftPalmPositionCache.get();
}

// thread-safe
glm::quat Avatar::getLeftPalmRotation() const {
    return _leftPalmRotationCache.get();
}

// thread-safe
glm::vec3 Avatar::getRightPalmPosition() const {
    return _rightPalmPositionCache.get();
}

// thread-safe
glm::quat Avatar::getRightPalmRotation() const {
    return _rightPalmRotationCache.get();
}

glm::vec3 Avatar::getUncachedLeftPalmPosition() const {
    assert(QThread::currentThread() == thread());  // main thread access only
    glm::quat leftPalmRotation;
    glm::vec3 leftPalmPosition;
    if (_skeletonModel->getLeftGrabPosition(leftPalmPosition)) {
        return leftPalmPosition;
    }
    // avatar didn't have a LeftHandMiddle1 joint, fall back on this:
    _skeletonModel->getJointRotationInWorldFrame(_skeletonModel->getLeftHandJointIndex(), leftPalmRotation);
    _skeletonModel->getLeftHandPosition(leftPalmPosition);
    leftPalmPosition += HAND_TO_PALM_OFFSET * glm::inverse(leftPalmRotation);
    return leftPalmPosition;
}

glm::quat Avatar::getUncachedLeftPalmRotation() const {
    assert(QThread::currentThread() == thread());  // main thread access only
    glm::quat leftPalmRotation;
    _skeletonModel->getJointRotationInWorldFrame(_skeletonModel->getLeftHandJointIndex(), leftPalmRotation);
    return leftPalmRotation;
}

glm::vec3 Avatar::getUncachedRightPalmPosition() const {
    assert(QThread::currentThread() == thread());  // main thread access only
    glm::quat rightPalmRotation;
    glm::vec3 rightPalmPosition;
    if (_skeletonModel->getRightGrabPosition(rightPalmPosition)) {
        return rightPalmPosition;
    }
    // avatar didn't have a RightHandMiddle1 joint, fall back on this:
    _skeletonModel->getJointRotationInWorldFrame(_skeletonModel->getRightHandJointIndex(), rightPalmRotation);
    _skeletonModel->getRightHandPosition(rightPalmPosition);
    rightPalmPosition += HAND_TO_PALM_OFFSET * glm::inverse(rightPalmRotation);
    return rightPalmPosition;
}

glm::quat Avatar::getUncachedRightPalmRotation() const {
    assert(QThread::currentThread() == thread());  // main thread access only
    glm::quat rightPalmRotation;
    _skeletonModel->getJointRotationInWorldFrame(_skeletonModel->getRightHandJointIndex(), rightPalmRotation);
    return rightPalmRotation;
}

void Avatar::setPositionViaScript(const glm::vec3& position) {
    setWorldPosition(position);
    updateAttitude(getWorldOrientation());
}

void Avatar::setOrientationViaScript(const glm::quat& orientation) {
    setWorldOrientation(orientation);
    updateAttitude(orientation);
}

void Avatar::updatePalms() {
    PerformanceTimer perfTimer("palms");
    // update thread-safe caches
    _leftPalmRotationCache.set(getUncachedLeftPalmRotation());
    _rightPalmRotationCache.set(getUncachedRightPalmRotation());
    _leftPalmPositionCache.set(getUncachedLeftPalmPosition());
    _rightPalmPositionCache.set(getUncachedRightPalmPosition());
}

void Avatar::setParentID(const QUuid& parentID) {
    if (!isMyAvatar()) {
        return;
    }
    QUuid initialParentID = getParentID();
    bool success;
    Transform beforeChangeTransform = getTransform(success);
    SpatiallyNestable::setParentID(parentID);
    if (success) {
        setTransform(beforeChangeTransform, success);
        if (!success) {
            qCDebug(avatars_renderer) << "Avatar::setParentID failed to reset avatar's location.";
        }
        if (initialParentID != parentID) {
            _parentChanged = usecTimestampNow();
        }
    }
}

void Avatar::setParentJointIndex(quint16 parentJointIndex) {
    if (!isMyAvatar()) {
        return;
    }
    bool success;
    Transform beforeChangeTransform = getTransform(success);
    SpatiallyNestable::setParentJointIndex(parentJointIndex);
    if (success) {
        setTransform(beforeChangeTransform, success);
        if (!success) {
            qCDebug(avatars_renderer) << "Avatar::setParentJointIndex failed to reset avatar's location.";
        }
    }
}

/*@jsdoc
 * Information about a joint in an avatar's skeleton hierarchy.
 * @typedef {object} SkeletonJoint
 * @property {string} name - Joint name.
 * @property {number} index - Joint index.
 * @property {number} parentIndex - Index of this joint's parent (<code>-1</code> if no parent).
 */
QList<QVariant> Avatar::getSkeleton() {
    SkeletonModelPointer skeletonModel = _skeletonModel;
    if (skeletonModel) {
        const Rig& rig = skeletonModel->getRig();
        AnimSkeleton::ConstPointer skeleton = rig.getAnimSkeleton();
        if (skeleton) {
            QList<QVariant> list;
            list.reserve(skeleton->getNumJoints());
            for (int i = 0; i < skeleton->getNumJoints(); i++) {
                QVariantMap obj;
                obj["name"] = skeleton->getJointName(i);
                obj["index"] = i;
                obj["parentIndex"] = skeleton->getParentIndex(i);
                list.push_back(obj);
            }
            return list;
        }
    }

    return QList<QVariant>();
}

void Avatar::addToScene(AvatarSharedPointer myHandle, const render::ScenePointer& scene) {
    if (scene) {
        auto nodelist = DependencyManager::get<NodeList>();
        if (showAvatars
            && !nodelist->isIgnoringNode(getSessionUUID())) {
            render::Transaction transaction;
            addToScene(myHandle, scene, transaction);
            scene->enqueueTransaction(transaction);
        }
    } else {
        qCWarning(avatars_renderer) << "Avatar::addAvatar() : Unexpected null scene, possibly during application shutdown";
    }
}

void Avatar::ensureInScene(AvatarSharedPointer self, const render::ScenePointer& scene) {
    if (!render::Item::isValidID(_renderItemID)) {
        addToScene(self, scene);
    }
}

// thread-safe
float Avatar::getEyeHeight() const {
    return getModelScale() * getUnscaledEyeHeight();
}

// thread-safe
float Avatar::getUnscaledEyeHeight() const {
    return _unscaledEyeHeightCache.get();
}

void Avatar::buildUnscaledEyeHeightCache() {
    float skeletonHeight = getUnscaledEyeHeightFromSkeleton();

    // Sanity check by looking at the model extents.
    Extents meshExtents = _skeletonModel->getUnscaledMeshExtents();
    float meshHeight = meshExtents.size().y;

    // if we determine the mesh is much larger then the skeleton, then we use the mesh height instead.
    // This helps prevent absurdly large avatars from exceeding the domain height limit.
    const float MESH_SLOP_RATIO = 1.5f;
    if (meshHeight > skeletonHeight * MESH_SLOP_RATIO) {
        _unscaledEyeHeightCache.set(meshHeight);
    } else {
        _unscaledEyeHeightCache.set(skeletonHeight);
    }
}

void Avatar::buildSpine2SplineRatioCache() {
    if (_skeletonModel) {
        auto& rig = _skeletonModel->getRig();
        AnimPose hipsRigDefaultPose = rig.getAbsoluteDefaultPose(rig.indexOfJoint("Hips"));
        AnimPose headRigDefaultPose(rig.getAbsoluteDefaultPose(rig.indexOfJoint("Head")));
        glm::vec3 basePosition = hipsRigDefaultPose.trans();
        glm::vec3 tipPosition = headRigDefaultPose.trans();
        glm::vec3 spine2Position = rig.getAbsoluteDefaultPose(rig.indexOfJoint("Spine2")).trans();

        glm::vec3 baseToTip = tipPosition - basePosition;
        float baseToTipLength = glm::length(baseToTip);
        glm::vec3 baseToTipNormal = baseToTip / baseToTipLength;
        glm::vec3 baseToSpine2 = spine2Position - basePosition;

        _spine2SplineRatio = glm::dot(baseToSpine2, baseToTipNormal) / baseToTipLength;

        CubicHermiteSplineFunctorWithArcLength defaultSpline(headRigDefaultPose.rot(), headRigDefaultPose.trans(), hipsRigDefaultPose.rot(), hipsRigDefaultPose.trans());

        // measure the total arc length along the spline
        float totalDefaultArcLength = defaultSpline.arcLength(1.0f);
        float t = defaultSpline.arcLengthInverse(_spine2SplineRatio * totalDefaultArcLength);
        glm::vec3 defaultSplineSpine2Translation = defaultSpline(t);

        _spine2SplineOffset = spine2Position - defaultSplineSpine2Translation;
    }

}

void Avatar::clearUnscaledEyeHeightCache() {
    _unscaledEyeHeightCache.set(DEFAULT_AVATAR_EYE_HEIGHT);
}

void Avatar::clearSpine2SplineRatioCache() {
    _spine2SplineRatio = DEFAULT_AVATAR_EYE_HEIGHT;
    _spine2SplineOffset = glm::vec3();
}

float Avatar::getUnscaledEyeHeightFromSkeleton() const {

    // TODO: if performance becomes a concern we can cache this value rather then computing it everytime.

    if (_skeletonModel) {
        return _skeletonModel->getRig().getUnscaledEyeHeight();
    } else {
        return DEFAULT_AVATAR_EYE_HEIGHT;
    }
}

AvatarTransit::Status Avatar::updateTransit(float deltaTime, const glm::vec3& avatarPosition, float avatarScale, const AvatarTransit::TransitConfig& config) {
    std::lock_guard<std::mutex> lock(_transitLock);
    _transit.setScale(avatarScale);
    return _transit.update(deltaTime, avatarPosition, config);
}

void Avatar::addMaterial(graphics::MaterialLayer material, const std::string& parentMaterialName) {
    std::lock_guard<std::mutex> lock(_materialsLock);
    _materials[parentMaterialName].push(material);
    if (_skeletonModel && _skeletonModel->fetchRenderItemIDs().size() > 0) {
        _skeletonModel->addMaterial(material, parentMaterialName);
    }
}

void Avatar::removeMaterial(graphics::MaterialPointer material, const std::string& parentMaterialName) {
    std::lock_guard<std::mutex> lock(_materialsLock);
    _materials[parentMaterialName].remove(material);
    if (_skeletonModel && _skeletonModel->fetchRenderItemIDs().size() > 0) {
        _skeletonModel->removeMaterial(material, parentMaterialName);
    }
}

void Avatar::processMaterials() {
    assert(_skeletonModel);
    std::lock_guard<std::mutex> lock(_materialsLock);
    for (auto& shapeMaterialPair : _materials) {
        auto material = shapeMaterialPair.second;
        while (!material.empty()) {
            _skeletonModel->addMaterial(material.top(), shapeMaterialPair.first);
            material.pop();
        }
    }
}

scriptable::ScriptableModelBase Avatar::getScriptableModel() {
    if (!_skeletonModel || !_skeletonModel->isLoaded()) {
        return scriptable::ScriptableModelBase();
    }
    auto result = _skeletonModel->getScriptableModel();
    result.objectID = getSessionUUID().isNull() ? AVATAR_SELF_ID : getSessionUUID();
    {
        std::lock_guard<std::mutex> lock(_materialsLock);
        result.appendMaterials(_materials);
    }
    return result;
}

void Avatar::clearAvatarGrabData(const QUuid& id) {
    AvatarData::clearAvatarGrabData(id);
    _avatarGrabsLock.withWriteLock([&] {
        if (_avatarGrabs.find(id) != _avatarGrabs.end()) {
            _grabsToDelete.push_back(id);
        }
    });
}

uint32_t Avatar::appendSubMetaItems(render::ItemIDs& subItems) {
    return _subItemLock.resultWithReadLock<uint32_t>([&] {
        uint32_t total = 0;

        if (_attachmentRenderIDs.size() > 0) {
            subItems.insert(subItems.end(), _attachmentRenderIDs.begin(), _attachmentRenderIDs.end());
            total += (uint32_t)_attachmentRenderIDs.size();
        }

        if (_descendantRenderIDs.size() > 0) {
            subItems.insert(subItems.end(), _descendantRenderIDs.begin(), _descendantRenderIDs.end());
            total += (uint32_t)_descendantRenderIDs.size();
        }

        return total;
    });
}

void Avatar::updateAttachmentRenderIDs() {
    _subItemLock.withWriteLock([&] {
        _attachmentRenderIDs.clear();
        for (auto& attachmentModel : _attachmentModels) {
            if (attachmentModel && attachmentModel->isRenderable()) {
                auto& metaSubItems = attachmentModel->fetchRenderItemIDs();
                _attachmentRenderIDs.insert(_attachmentRenderIDs.end(), metaSubItems.begin(), metaSubItems.end());
            }
        }
    });
}

void Avatar::updateDescendantRenderIDs() {
    _subItemLock.withWriteLock([&] {
        auto oldRenderingDescendantEntityIDs = _renderingDescendantEntityIDs;
        _renderingDescendantEntityIDs.clear();
        _descendantRenderIDs.clear();
        auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
        EntityTreePointer entityTree = entityTreeRenderer ? entityTreeRenderer->getTree() : nullptr;
        if (entityTree) {
            entityTree->withReadLock([&] {
                forEachDescendant([&](SpatiallyNestablePointer object) {
                    if (object && object->getNestableType() == NestableType::Entity) {
                        EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                        if (entity->isVisible()) {
                            auto id = object->getID();
                            _renderingDescendantEntityIDs.insert(id);
                            oldRenderingDescendantEntityIDs.erase(id);
                            entity->setCullWithParent(true);

                            auto renderer = entityTreeRenderer->renderableForEntityId(id);
                            if (renderer) {
                                render::ItemIDs renderableSubItems;
                                uint32_t numRenderableSubItems = renderer->metaFetchMetaSubItems(renderableSubItems);
                                if (numRenderableSubItems > 0) {
                                    _descendantRenderIDs.insert(_descendantRenderIDs.end(), renderableSubItems.begin(), renderableSubItems.end());
                                }
                            }
                        }
                    }
                });

                for (auto& oldRenderingDescendantEntityID : oldRenderingDescendantEntityIDs) {
                    auto entity = entityTree->findEntityByEntityItemID(oldRenderingDescendantEntityID);
                    if (entity) {
                        entity->setCullWithParent(false);
                    }
                }
            });
        }
    });
}
