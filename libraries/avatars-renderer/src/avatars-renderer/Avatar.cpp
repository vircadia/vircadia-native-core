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

#include "Logging.h"

using namespace std;

const int   NUM_BODY_CONE_SIDES = 9;
const float CHAT_MESSAGE_SCALE = 0.0015f;
const float CHAT_MESSAGE_HEIGHT = 0.1f;
const float DISPLAYNAME_FADE_TIME = 0.5f;
const float DISPLAYNAME_FADE_FACTOR = pow(0.01f, 1.0f / DISPLAYNAME_FADE_TIME);
const float DISPLAYNAME_ALPHA = 1.0f;
const float DISPLAYNAME_BACKGROUND_ALPHA = 0.4f;
const glm::vec3 HAND_TO_PALM_OFFSET(0.0f, 0.12f, 0.08f);

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar) {
        return ItemKey::Builder::opaqueShape().withTypeMeta();
    }
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar) {
        return static_pointer_cast<Avatar>(avatar)->getBounds();
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
        if (avatarPtr->getSkeletonModel()) {
            auto& metaSubItems = avatarPtr->getSkeletonModel()->fetchRenderItemIDs();
            subItems.insert(subItems.end(), metaSubItems.begin(), metaSubItems.end());
            return (uint32_t) metaSubItems.size();
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

static bool showOtherLookAtVectors = false;
void Avatar::setShowOtherLookAtVectors(bool showOthers) {
    showOtherLookAtVectors = showOthers;
}

static bool showCollisionShapes = false;
void Avatar::setShowCollisionShapes(bool render) {
    showCollisionShapes = render;
}

static bool showNamesAboveHeads = false;
void Avatar::setShowNamesAboveHeads(bool show) {
    showNamesAboveHeads = show;
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
}

Avatar::~Avatar() {
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (entityTree) {
        entityTree->withWriteLock([&] {
            AvatarEntityMap avatarEntities = getAvatarEntityData();
            for (auto entityID : avatarEntities.keys()) {
                entityTree->deleteEntity(entityID, true, true);
            }
        });
    }

    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_nameRectGeometryID);
        geometryCache->releaseID(_leftPointerGeometryID);
        geometryCache->releaseID(_rightPointerGeometryID);
    }
}

void Avatar::init() {
    getHead()->init();
    _skeletonModel->init();
    _initialized = true;
}

glm::vec3 Avatar::getChestPosition() const {
    // for now, let's just assume that the "chest" is halfway between the root and the neck
    glm::vec3 neckPosition;
    return _skeletonModel->getNeckPosition(neckPosition) ? (getPosition() + neckPosition) * 0.5f : getPosition();
}

glm::vec3 Avatar::getNeckPosition() const {
    glm::vec3 neckPosition;
    return _skeletonModel->getNeckPosition(neckPosition) ? neckPosition : getPosition();
}

AABox Avatar::getBounds() const {
    if (!_skeletonModel->isRenderable() || _skeletonModel->needsFixupInScene()) {
        // approximately 2m tall, scaled to user request.
        return AABox(getPosition() - glm::vec3(getModelScale()), getModelScale() * 2.0f);
    }
    return _skeletonModel->getRenderableMeshBound();
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
        const float MIN_RELATIVE_ERROR = 0.03f;
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
        _isAnimatingScale = true;
    }
}

void Avatar::updateAvatarEntities() {
    PerformanceTimer perfTimer("attachments");
    // - if queueEditEntityMessage sees clientOnly flag it does _myAvatar->updateAvatarEntity()
    // - updateAvatarEntity saves the bytes and sets _avatarEntityDataLocallyEdited
    // - MyAvatar::update notices _avatarEntityDataLocallyEdited and calls sendIdentityPacket
    // - sendIdentityPacket sends the entity bytes to the server which relays them to other interfaces
    // - AvatarHashMap::processAvatarIdentityPacket on other interfaces call avatar->setAvatarEntityData()
    // - setAvatarEntityData saves the bytes and sets _avatarEntityDataChanged = true
    // - (My)Avatar::simulate notices _avatarEntityDataChanged and here we are...

    if (!_avatarEntityDataChanged) {
        return;
    }

    if (getID() == QUuid()) {
        return; // wait until MyAvatar gets an ID before doing this.
    }

    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (!entityTree) {
        return;
    }

    QScriptEngine scriptEngine;
    entityTree->withWriteLock([&] {
        AvatarEntityMap avatarEntities = getAvatarEntityData();
        AvatarEntityMap::const_iterator dataItr = avatarEntities.begin();
        while (dataItr != avatarEntities.end()) {
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
                // remember this hash for the future
                stateItr = _avatarEntityDataHashes.insert(entityID, AvatarEntityDataHash(newHash));
            }
            ++dataItr;

            // see EntityEditPacketSender::queueEditEntityMessage for the other end of this.  unpack properties
            // and either add or update the entity.
            QJsonDocument jsonProperties = QJsonDocument::fromBinaryData(data);
            if (!jsonProperties.isObject()) {
                qCDebug(avatars_renderer) << "got bad avatarEntity json" << QString(data.toHex());
                continue;
            }

            QVariant variantProperties = jsonProperties.toVariant();
            QVariantMap asMap = variantProperties.toMap();
            QScriptValue scriptProperties = variantMapToScriptValue(asMap, scriptEngine);
            EntityItemProperties properties;
            EntityItemPropertiesFromScriptValueHonorReadOnly(scriptProperties, properties);
            properties.setClientOnly(true);
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
                qCDebug(avatars_renderer) << "removing entity script from avatar attached entity:" << entityID << "old script:" << attachedScript;
                QString noScript;
                properties.setScript(noScript);
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
                if (entityTree->updateEntity(entityID, properties)) {
                    entity->updateLastEditedFromRemote();
                } else {
                    success = false;
                }
            } else {
                entity = entityTree->addEntity(entityID, properties);
                if (!entity) {
                    success = false;
                }
            }
            stateItr.value().success = success;
        }

        AvatarEntityIDs recentlyDettachedAvatarEntities = getAndClearRecentlyDetachedIDs();
        if (!recentlyDettachedAvatarEntities.empty()) {
            // only lock this thread when absolutely necessary
            _avatarEntitiesLock.withReadLock([&] {
                foreach (auto entityID, recentlyDettachedAvatarEntities) {
                    if (!_avatarEntityData.contains(entityID)) {
                        entityTree->deleteEntity(entityID, true, true);
                    }
                }
            });

            // remove stale data hashes
            foreach (auto entityID, recentlyDettachedAvatarEntities) {
                MapOfAvatarEntityDataHashes::iterator stateItr = _avatarEntityDataHashes.find(entityID);
                if (stateItr != _avatarEntityDataHashes.end()) {
                    _avatarEntityDataHashes.erase(stateItr);
                }
            }
        }
    });

    setAvatarEntityDataChanged(false);
}

void Avatar::simulate(float deltaTime, bool inView) {
    PROFILE_RANGE(simulation, "simulate");

    _simulationRate.increment();
    if (inView) {
        _simulationInViewRate.increment();
    }

    PerformanceTimer perfTimer("simulate");
    {
        PROFILE_RANGE(simulation, "updateJoints");
        if (inView) {
            Head* head = getHead();
            if (_hasNewJointData) {
                _skeletonModel->getRig().copyJointsFromJointData(_jointData);
                glm::mat4 rootTransform = glm::scale(_skeletonModel->getScale()) * glm::translate(_skeletonModel->getOffset());
                _skeletonModel->getRig().computeExternalPoses(rootTransform);
                _jointDataSimulationRate.increment();

                _skeletonModel->simulate(deltaTime, true);

                locationChanged(); // joints changed, so if there are any children, update them.
                _hasNewJointData = false;

                glm::vec3 headPosition = getPosition();
                if (!_skeletonModel->getHeadPosition(headPosition)) {
                    headPosition = getPosition();
                }
                head->setPosition(headPosition);
            }
            head->setScale(getModelScale());
            head->simulate(deltaTime);
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
            _displayNameAlpha = 1 - (1 - _displayNameAlpha) * coef;
        }
        _displayNameAlpha = abs(_displayNameAlpha - _displayNameTargetAlpha) < 0.01f ? _displayNameTargetAlpha : _displayNameAlpha;
    }

    {
        PROFILE_RANGE(simulation, "misc");
        measureMotionDerivatives(deltaTime);
        simulateAttachments(deltaTime);
        updatePalms();
    }
    {
        PROFILE_RANGE(simulation, "entities");
        updateAvatarEntities();
    }
}

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
    setPosition(newPosition);
    _positionDeltaAccumulator = glm::vec3(0.0f);
    setVelocity(glm::vec3(0.0f));
    _lastVelocity = glm::vec3(0.0f);
}

void Avatar::updateAttitude(const glm::quat& orientation) {
    _skeletonModel->updateAttitude(orientation);
    _worldUpDirection = orientation * Vectors::UNIT_Y;
}

void Avatar::applyPositionDelta(const glm::vec3& delta) {
    setPosition(getPosition() + delta);
    _positionDeltaAccumulator += delta;
}

void Avatar::measureMotionDerivatives(float deltaTime) {
    PerformanceTimer perfTimer("derivatives");
    // linear
    float invDeltaTime = 1.0f / deltaTime;
    // Floating point error prevents us from computing velocity in a naive way
    // (e.g. vel = (pos - oldPos) / dt) so instead we use _positionOffsetAccumulator.
    glm::vec3 velocity = _positionDeltaAccumulator * invDeltaTime;
    _positionDeltaAccumulator = glm::vec3(0.0f);
    _acceleration = (velocity - _lastVelocity) * invDeltaTime;
    _lastVelocity = velocity;
    setVelocity(velocity);

    // angular
    glm::quat orientation = getOrientation();
    glm::quat delta = glm::inverse(_lastOrientation) * orientation;
    glm::vec3 angularVelocity = glm::axis(delta) * glm::angle(delta) * invDeltaTime;
    setAngularVelocity(angularVelocity);
    _lastOrientation = getOrientation();
}

enum TextRendererType {
    CHAT,
    DISPLAYNAME
};

static TextRenderer3D* textRenderer(TextRendererType type) {
    static TextRenderer3D* chatRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY, -1,
        false, SHADOW_EFFECT);
    static TextRenderer3D* displayNameRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY);

    switch(type) {
    case CHAT:
        return chatRenderer;
    case DISPLAYNAME:
        return displayNameRenderer;
    }

    return displayNameRenderer;
}

void Avatar::addToScene(AvatarSharedPointer self, const render::ScenePointer& scene, render::Transaction& transaction) {
    auto avatarPayload = new render::Payload<AvatarData>(self);
    auto avatarPayloadPointer = Avatar::PayloadPointer(avatarPayload);

    if (_renderItemID == render::Item::INVALID_ITEM_ID) {
        _renderItemID = scene->allocateID();
    }
    transaction.resetItem(_renderItemID, avatarPayloadPointer);
    _skeletonModel->addToScene(scene, transaction);
    for (auto& attachmentModel : _attachmentModels) {
        attachmentModel->addToScene(scene, transaction);
    }

    _mustFadeIn = true;
}

void Avatar::fadeIn(render::ScenePointer scene) {
    render::Transaction transaction;
    fade(transaction, render::Transition::USER_ENTER_DOMAIN);
    scene->enqueueTransaction(transaction);
}

void Avatar::fadeOut(render::ScenePointer scene, KillAvatarReason reason) {
    render::Transition::Type transitionType = render::Transition::USER_LEAVE_DOMAIN;
    render::Transaction transaction;

    if (reason == KillAvatarReason::YourAvatarEnteredTheirBubble) {
        transitionType = render::Transition::BUBBLE_ISECT_TRESPASSER;
    }
    else if (reason == KillAvatarReason::TheirAvatarEnteredYourBubble) {
        transitionType = render::Transition::BUBBLE_ISECT_OWNER;
    }
    fade(transaction, transitionType);
    scene->enqueueTransaction(transaction);
}

void Avatar::fade(render::Transaction& transaction, render::Transition::Type type) {
    transaction.addTransitionToItem(_renderItemID, type);
    for (auto& attachmentModel : _attachmentModels) {
        for (auto itemId : attachmentModel->fetchRenderItemIDs()) {
            transaction.addTransitionToItem(itemId, type, _renderItemID);
        }
    }
    _isFading = true;
}

void Avatar::updateFadingStatus(render::ScenePointer scene) {
    render::Transaction transaction;
    transaction.queryTransitionOnItem(_renderItemID, [this](render::ItemID id, const render::Transition* transition) {
        if (transition == nullptr || transition->isFinished) {
            _isFading = false;
        }
    });
    scene->enqueueTransaction(transaction);
}

void Avatar::removeFromScene(AvatarSharedPointer self, const render::ScenePointer& scene, render::Transaction& transaction) {
    transaction.removeItem(_renderItemID);
    render::Item::clearID(_renderItemID);
    _skeletonModel->removeFromScene(scene, transaction);
    for (auto& attachmentModel : _attachmentModels) {
        attachmentModel->removeFromScene(scene, transaction);
    }
}

void Avatar::updateRenderItem(render::Transaction& transaction) {
    if (render::Item::isValidID(_renderItemID)) {
        transaction.updateItem<render::Payload<AvatarData>>(_renderItemID, [](render::Payload<AvatarData>& p) {});
    }
}

void Avatar::postUpdate(float deltaTime, const render::ScenePointer& scene) {

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
}

void Avatar::render(RenderArgs* renderArgs) {
    auto& batch = *(renderArgs->_batch);
    PROFILE_RANGE_BATCH(batch, __FUNCTION__);

    glm::vec3 viewPos = renderArgs->getViewFrustum().getPosition();
    const float MAX_DISTANCE_SQUARED_FOR_SHOWING_POINTING_LASERS = 100.0f; // 10^2
    if (glm::distance2(viewPos, getPosition()) < MAX_DISTANCE_SQUARED_FOR_SHOWING_POINTING_LASERS) {
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
                geometryCache->bindSimpleProgram(batch);
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
                geometryCache->bindSimpleProgram(batch);
                geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, laserLength, 0.0f), laserColor, _rightPointerGeometryID);
            }
        }
    }

    ViewFrustum frustum = renderArgs->getViewFrustum();
    if (!frustum.sphereIntersectsFrustum(getPosition(), getBoundingRadius())) {
        return;
    }

    if (showCollisionShapes && shouldRenderHead(renderArgs) && _skeletonModel->isRenderable()) {
        PROFILE_RANGE_BATCH(batch, __FUNCTION__":skeletonBoundingCollisionShapes");
        const float BOUNDING_SHAPE_ALPHA = 0.7f;
        _skeletonModel->renderBoundingCollisionShapes(renderArgs, *renderArgs->_batch, getModelScale(), BOUNDING_SHAPE_ALPHA);
    }

    if (showReceiveStats || showNamesAboveHeads) {
        glm::vec3 toTarget = frustum.getPosition() - getPosition();
        float distanceToTarget = glm::length(toTarget);
        const float DISPLAYNAME_DISTANCE = 20.0f;
        updateDisplayNameAlpha(distanceToTarget < DISPLAYNAME_DISTANCE);
        if (!isMyAvatar() || renderArgs->_cameraMode != (int8_t)CAMERA_MODE_FIRST_PERSON) {
            auto& frustum = renderArgs->getViewFrustum();
            auto textPosition = getDisplayNamePosition();
            if (frustum.pointIntersectsFrustum(textPosition)) {
                renderDisplayName(batch, frustum, textPosition);
            }
        }
    }
}

void Avatar::fixupModelsInScene(const render::ScenePointer& scene) {
    bool canTryFade{ false };

    _attachmentsToDelete.clear();

    // check to see if when we added our models to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::Transaction transaction;
    if (_skeletonModel->isRenderable() && _skeletonModel->needsFixupInScene()) {
        _skeletonModel->removeFromScene(scene, transaction);
        _skeletonModel->addToScene(scene, transaction);
        canTryFade = true;
    }
    for (auto attachmentModel : _attachmentModels) {
        if (attachmentModel->isRenderable() && attachmentModel->needsFixupInScene()) {
            attachmentModel->removeFromScene(scene, transaction);
            attachmentModel->addToScene(scene, transaction);
        }
    }

    if (_mustFadeIn && canTryFade) {
        // Do it now to be sure all the sub items are ready and the fade is sent to them too
        fade(transaction, render::Transition::USER_ENTER_DOMAIN);
        _mustFadeIn = false;
    }

    for (auto attachmentModelToRemove : _attachmentsToRemove) {
        attachmentModelToRemove->removeFromScene(scene, transaction);
    }
    _attachmentsToDelete.insert(_attachmentsToDelete.end(), _attachmentsToRemove.begin(), _attachmentsToRemove.end());
    _attachmentsToRemove.clear();
    scene->enqueueTransaction(transaction);
}

bool Avatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    return true;
}

// virtual
void Avatar::simulateAttachments(float deltaTime) {
    PerformanceTimer perfTimer("attachments");
    for (int i = 0; i < (int)_attachmentModels.size(); i++) {
        const AttachmentData& attachment = _attachmentData.at(i);
        auto& model = _attachmentModels.at(i);
        int jointIndex = getJointIndex(attachment.jointName);
        glm::vec3 jointPosition;
        glm::quat jointRotation;
        if (attachment.isSoft) {
            // soft attachments do not have transform offsets
            model->setTranslation(getPosition());
            model->setRotation(getOrientation() * Quaternions::Y_180);
            model->simulate(deltaTime);
        } else {
            if (_skeletonModel->getJointPositionInWorldFrame(jointIndex, jointPosition) &&
                _skeletonModel->getJointRotationInWorldFrame(jointIndex, jointRotation)) {
                model->setTranslation(jointPosition + jointRotation * attachment.translation * getModelScale());
                model->setRotation(jointRotation * attachment.rotation);
                float scale = getModelScale() * attachment.scale;
                model->setScaleToFit(true, model->getNaturalDimensions() * scale, true); // hack to force rescale
                model->setSnapModelToCenter(false); // hack to force resnap
                model->setSnapModelToCenter(true);
                model->simulate(deltaTime);
            }
        }
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

        DEBUG_VALUE("_position =", getPosition());
        DEBUG_VALUE("size =", size);
        namePosition = getPosition() + bodyUpDirection * (size * HEAD_PROPORTION);
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

void Avatar::renderDisplayName(gpu::Batch& batch, const ViewFrustum& view, const glm::vec3& textPosition) const {
    PROFILE_RANGE_BATCH(batch, __FUNCTION__);

    bool shouldShowReceiveStats = showReceiveStats && !isMyAvatar();

    // If we have nothing to draw, or it's totally transparent, or it's too close or behind the camera, return
    static const float CLIP_DISTANCE = 0.2f;
    if ((_displayName.isEmpty() && !shouldShowReceiveStats) || _displayNameAlpha == 0.0f
        || (glm::dot(view.getDirection(), getDisplayNamePosition() - view.getPosition()) <= CLIP_DISTANCE)) {
        return;
    }
    auto renderer = textRenderer(DISPLAYNAME);

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
    const glm::vec2 extent = renderer->computeExtent(renderedDisplayName);
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
            DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch, false, false, true, true, true);
            DependencyManager::get<GeometryCache>()->renderBevelCornersRect(batch, left, bottom, width, height,
                bevelDistance, backgroundColor, _nameRectGeometryID);
        }

        // Render actual name
        QByteArray nameUTF8 = renderedDisplayName.toLocal8Bit();

        // Render text slightly in front to avoid z-fighting
        textTransform.postTranslate(glm::vec3(0.0f, 0.0f, SLIGHTLY_IN_FRONT * renderer->getFontSize()));
        batch.setModelTransform(textTransform);
        {
            PROFILE_RANGE_BATCH(batch, __FUNCTION__":renderText");
            renderer->draw(batch, text_x, -text_y, nameUTF8.data(), textColor);
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
    return getPosition() + getOrientation() * FLIP * _skeletonOffset;
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
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            glm::mat4 avatarMatrix = getLocalTransform().getMatrix();
            glm::mat4 finalMat = glm::inverse(avatarMatrix) * sensorToWorldMatrix;
            return glmExtractRotation(finalMat);
        }
        case CONTROLLER_LEFTHAND_INDEX: {
            Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
            return controllerLeftHandTransform.getRotation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
            return controllerRightHandTransform.getRotation();
        }
        case CAMERA_MATRIX_INDEX: {
            glm::quat rotation;
            if (_skeletonModel && _skeletonModel->isActive()) {
                int headJointIndex = _skeletonModel->getFBXGeometry().headJointIndex;
                if (headJointIndex >= 0) {
                    _skeletonModel->getAbsoluteJointRotationInRigFrame(headJointIndex, rotation);
                }
            }
            return Quaternions::Y_180 * rotation * Quaternions::Y_180;
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
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            glm::mat4 avatarMatrix = getLocalTransform().getMatrix();
            glm::mat4 finalMat = glm::inverse(avatarMatrix) * sensorToWorldMatrix;
            return extractTranslation(finalMat);
        }
        case CONTROLLER_LEFTHAND_INDEX: {
            Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
            return controllerLeftHandTransform.getTranslation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
            return controllerRightHandTransform.getTranslation();
        }
        case CAMERA_MATRIX_INDEX: {
            glm::vec3 translation;
            if (_skeletonModel && _skeletonModel->isActive()) {
                int headJointIndex = _skeletonModel->getFBXGeometry().headJointIndex;
                if (headJointIndex >= 0) {
                    _skeletonModel->getAbsoluteJointTranslationInRigFrame(headJointIndex, translation);
                }
            }
            return Quaternions::Y_180 * translation * Quaternions::Y_180;
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
                if (_skeletonModel && _skeletonModel->isActive()) {
                    _modelJointIndicesCache = _skeletonModel->getFBXGeometry().jointIndices;
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
        if (_modelJointIndicesCache.contains(name)) {
            result = _modelJointIndicesCache[name] - 1;
        }
    });
    return result;
}

QStringList Avatar::getJointNames() const {
    QStringList result;
    withValidJointIndicesCache([&]() {
        // find out how large the vector needs to be
        int maxJointIndex = -1;
        QHashIterator<QString, int> k(_modelJointIndicesCache);
        while (k.hasNext()) {
            k.next();
            int index = k.value();
            if (index > maxJointIndex) {
                maxJointIndex = index;
            }
        }
        // iterate through the hash and put joint names
        // into the vector at their indices
        QVector<QString> resultVector(maxJointIndex+1);
        QHashIterator<QString, int> i(_modelJointIndicesCache);
        while (i.hasNext()) {
            i.next();
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
    positionToScale = getPosition() + getModelScale() * (positionToScale - getPosition());
}

void Avatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    AvatarData::setSkeletonModelURL(skeletonModelURL);
    if (QThread::currentThread() == thread()) {
        _skeletonModel->setURL(_skeletonModelURL);
    } else {
        QMetaObject::invokeMethod(_skeletonModel.get(), "setURL", Qt::QueuedConnection, Q_ARG(QUrl, _skeletonModelURL));
    }
}

void Avatar::setModelURLFinished(bool success) {
    invalidateJointIndicesCache();

    if (!success && _skeletonModelURL != AvatarData::defaultFullAvatarModelUrl()) {
        const int MAX_SKELETON_DOWNLOAD_ATTEMPTS = 4; // NOTE: we don't want to be as generous as ResourceCache is, we only want 4 attempts
        if (_skeletonModel->getResourceDownloadAttemptsRemaining() <= 0 ||
            _skeletonModel->getResourceDownloadAttempts() > MAX_SKELETON_DOWNLOAD_ATTEMPTS) {
            qCWarning(avatars_renderer) << "Using default after failing to load Avatar model: " << _skeletonModelURL
                                    << "after" << _skeletonModel->getResourceDownloadAttempts() << "attempts.";
            // call _skeletonModel.setURL, but leave our copy of _skeletonModelURL alone.  This is so that
            // we don't redo this every time we receive an identity packet from the avatar with the bad url.
            QMetaObject::invokeMethod(_skeletonModel.get(), "setURL",
                Qt::QueuedConnection, Q_ARG(QUrl, AvatarData::defaultFullAvatarModelUrl()));
        } else {
            qCWarning(avatars_renderer) << "Avatar model: " << _skeletonModelURL
                    << "failed to load... attempts:" << _skeletonModel->getResourceDownloadAttempts()
                    << "out of:" << MAX_SKELETON_DOWNLOAD_ATTEMPTS;
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
        _attachmentsToRemove.push_back(attachmentModel);
    }

    for (int i = 0; i < attachmentData.size(); i++) {
        if (i == (int)_attachmentModels.size()) {
            // if number of attachments has been increased, we need to allocate a new model
            _attachmentModels.push_back(allocateAttachmentModel(attachmentData[i].isSoft, _skeletonModel->getRig(), isMyAvatar()));
        }
        else if (i < oldAttachmentData.size() && oldAttachmentData[i].isSoft != attachmentData[i].isSoft) {
            // if the attachment has changed type, we need to re-allocate a new one.
            _attachmentsToRemove.push_back(_attachmentModels[i]);
            _attachmentModels[i] = allocateAttachmentModel(attachmentData[i].isSoft, _skeletonModel->getRig(), isMyAvatar());
        }
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
    glm::vec3 oldPosition = getPosition();

    int bytesRead = AvatarData::parseDataFromBuffer(buffer);

    const float MOVE_DISTANCE_THRESHOLD = 0.001f;
    _moving = glm::distance(oldPosition, getPosition()) > MOVE_DISTANCE_THRESHOLD;
    if (_moving) {
        addPhysicsFlags(Simulation::DIRTY_POSITION);
    }
    if (_moving || _hasNewJointData) {
        locationChanged();
    }

    return bytesRead;
}

int Avatar::_jointConesID = GeometryCache::UNKNOWN_ID;

// render a makeshift cone section that serves as a body part connecting joint spheres
void Avatar::renderJointConnectingCone(gpu::Batch& batch, glm::vec3 position1, glm::vec3 position2,
                                            float radius1, float radius2, const glm::vec4& color) {

    auto geometryCache = DependencyManager::get<GeometryCache>();

    if (_jointConesID == GeometryCache::UNKNOWN_ID) {
        _jointConesID = geometryCache->allocateID();
    }

    glm::vec3 axis = position2 - position1;
    float length = glm::length(axis);

    if (length > 0.0f) {

        axis /= length;

        glm::vec3 perpSin = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 perpCos = glm::normalize(glm::cross(axis, perpSin));
        perpSin = glm::cross(perpCos, axis);

        float angleb = 0.0f;
        QVector<glm::vec3> points;

        for (int i = 0; i < NUM_BODY_CONE_SIDES; i ++) {

            // the rectangles that comprise the sides of the cone section are
            // referenced by "a" and "b" in one dimension, and "1", and "2" in the other dimension.
            int anglea = angleb;
            angleb = ((float)(i+1) / (float)NUM_BODY_CONE_SIDES) * TWO_PI;

            float sa = sinf(anglea);
            float sb = sinf(angleb);
            float ca = cosf(anglea);
            float cb = cosf(angleb);

            glm::vec3 p1a = position1 + perpSin * sa * radius1 + perpCos * ca * radius1;
            glm::vec3 p1b = position1 + perpSin * sb * radius1 + perpCos * cb * radius1;
            glm::vec3 p2a = position2 + perpSin * sa * radius2 + perpCos * ca * radius2;
            glm::vec3 p2b = position2 + perpSin * sb * radius2 + perpCos * cb * radius2;

            points << p1a << p1b << p2a << p1b << p2a << p2b;
        }

        PROFILE_RANGE_BATCH(batch, __FUNCTION__);
        // TODO: this is really inefficient constantly recreating these vertices buffers. It would be
        // better if the avatars cached these buffers for each of the joints they are rendering
        geometryCache->updateVertices(_jointConesID, points, color);
        geometryCache->renderVertices(batch, gpu::TRIANGLES, _jointConesID);
    }
}

float Avatar::getSkeletonHeight() const {
    Extents extents = _skeletonModel->getBindExtents();
    return extents.maximum.y - extents.minimum.y;
}

float Avatar::getHeadHeight() const {
    Extents extents = _skeletonModel->getMeshExtents();
    glm::vec3 neckPosition;
    if (!extents.isEmpty() && extents.isValid() && _skeletonModel->getNeckPosition(neckPosition)) {
        return extents.maximum.y / 2.0f - neckPosition.y + getPosition().y;
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

// virtual
void Avatar::computeShapeInfo(ShapeInfo& shapeInfo) {
    float uniformScale = getModelScale();
    shapeInfo.setCapsuleY(uniformScale * _skeletonModel->getBoundingCapsuleRadius(),
            0.5f * uniformScale *  _skeletonModel->getBoundingCapsuleHeight());
    shapeInfo.setOffset(uniformScale * _skeletonModel->getBoundingCapsuleOffset());
}

void Avatar::getCapsule(glm::vec3& start, glm::vec3& end, float& radius) {
    ShapeInfo shapeInfo;
    computeShapeInfo(shapeInfo);
    glm::vec3 halfExtents = shapeInfo.getHalfExtents(); // x = radius, y = halfHeight
    start = getPosition() - glm::vec3(0, halfExtents.y, 0) + shapeInfo.getOffset();
    end = getPosition() + glm::vec3(0, halfExtents.y, 0) + shapeInfo.getOffset();
    radius = halfExtents.x;
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

// virtual
void Avatar::rebuildCollisionShape() {
    addPhysicsFlags(Simulation::DIRTY_SHAPE);
}

void Avatar::setPhysicsCallback(AvatarPhysicsCallback cb) {
    _physicsCallback = cb;
}

void Avatar::addPhysicsFlags(uint32_t flags) {
    if (_physicsCallback) {
        _physicsCallback(flags);
    }
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
    setPosition(position);
    updateAttitude(getOrientation());
}

void Avatar::setOrientationViaScript(const glm::quat& orientation) {
    setOrientation(orientation);
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

float Avatar::getEyeHeight() const {

    if (QThread::currentThread() != thread()) {
        float result = DEFAULT_AVATAR_EYE_HEIGHT;
        BLOCKING_INVOKE_METHOD(const_cast<Avatar*>(this), "getHeight", Q_RETURN_ARG(float, result));
        return result;
    }

    // TODO: if performance becomes a concern we can cache this value rather then computing it everytime.
    // Makes assumption that the y = 0 plane in geometry is the ground plane.
    // We also make that assumption in Rig::computeAvatarBoundingCapsule()
    float avatarScale = getModelScale();
    if (_skeletonModel) {
        auto& rig = _skeletonModel->getRig();
        int headTopJoint = rig.indexOfJoint("HeadTop_End");
        int headJoint = rig.indexOfJoint("Head");
        int eyeJoint = rig.indexOfJoint("LeftEye") != -1 ? rig.indexOfJoint("LeftEye") : rig.indexOfJoint("RightEye");
        int toeJoint = rig.indexOfJoint("LeftToeBase") != -1 ? rig.indexOfJoint("LeftToeBase") : rig.indexOfJoint("RightToeBase");
        if (eyeJoint >= 0 && toeJoint >= 0) {
            // measure from eyes to toes.
            float eyeHeight = rig.getAbsoluteDefaultPose(eyeJoint).trans().y - rig.getAbsoluteDefaultPose(toeJoint).trans().y;
            return eyeHeight;
        } else if (eyeJoint >= 0) {
            // measure eyes to y = 0 plane.
            float groundHeight = transformPoint(rig.getGeometryToRigTransform(), glm::vec3(0.0f)).y;
            float eyeHeight = rig.getAbsoluteDefaultPose(eyeJoint).trans().y - groundHeight;
            return eyeHeight;
        } else if (headTopJoint >= 0 && toeJoint >= 0) {
            // measure toe to top of head.  Note: default poses already include avatar scale factor
            const float ratio = DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD / DEFAULT_AVATAR_HEIGHT;
            float height = rig.getAbsoluteDefaultPose(headTopJoint).trans().y - rig.getAbsoluteDefaultPose(toeJoint).trans().y;
            return height - height * ratio;
        } else if (headTopJoint >= 0) {
            const float ratio = DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD / DEFAULT_AVATAR_HEIGHT;
            float groundHeight = transformPoint(rig.getGeometryToRigTransform(), glm::vec3(0.0f)).y;
            float headHeight = rig.getAbsoluteDefaultPose(headTopJoint).trans().y - groundHeight;
            return headHeight - headHeight * ratio;
        } else if (headJoint >= 0) {
            float groundHeight = transformPoint(rig.getGeometryToRigTransform(), glm::vec3(0.0f)).y;
            const float DEFAULT_AVATAR_NECK_TO_EYE = DEFAULT_AVATAR_NECK_TO_TOP_OF_HEAD - DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD;
            const float ratio = DEFAULT_AVATAR_NECK_TO_EYE / DEFAULT_AVATAR_NECK_HEIGHT;
            float neckHeight = rig.getAbsoluteDefaultPose(headJoint).trans().y - groundHeight;
            return neckHeight + neckHeight * ratio;
        } else {
            return avatarScale * DEFAULT_AVATAR_EYE_HEIGHT;
        }
    } else {
        return avatarScale * DEFAULT_AVATAR_EYE_HEIGHT;
    }
}
