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

#include <vector>

#include <QDesktopWidget>
#include <QWindow>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_query.hpp>

#include <DeferredLightingEffect.h>
#include <GeometryUtil.h>
#include <LODManager.h>
#include <NodeList.h>
#include <NumericalConstants.h>
#include <OctreeUtils.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>
#include <SharedUtil.h>
#include <TextRenderer3D.h>
#include <TextureCache.h>
#include <VariantMapToScriptValue.h>
#include <DebugDraw.h>

#include "Application.h"
#include "Avatar.h"
#include "AvatarManager.h"
#include "AvatarMotionState.h"
#include "Head.h"
#include "Menu.h"
#include "Physics.h"
#include "Util.h"
#include "world.h"
#include "InterfaceLogging.h"
#include "SoftAttachmentModel.h"
#include <Rig.h>

using namespace std;

const glm::vec3 DEFAULT_UP_DIRECTION(0.0f, 1.0f, 0.0f);
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
        return ItemKey::Builder::opaqueShape();
    }
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar) {
        return static_pointer_cast<Avatar>(avatar)->getBounds();
    }
    template <> void payloadRender(const AvatarSharedPointer& avatar, RenderArgs* args) {
        auto avatarPtr = static_pointer_cast<Avatar>(avatar);
        if (avatarPtr->isInitialized() && args) {
            PROFILE_RANGE_BATCH(*args->_batch, "renderAvatarPayload");
            avatarPtr->render(args, qApp->getCamera()->getPosition());
        }
    }
}

Avatar::Avatar(RigPointer rig) :
    AvatarData(),
    _skeletonOffset(0.0f),
    _bodyYawDelta(0.0f),
    _positionDeltaAccumulator(0.0f),
    _lastVelocity(0.0f),
    _acceleration(0.0f),
    _lastAngularVelocity(0.0f),
    _lastOrientation(),
    _worldUpDirection(DEFAULT_UP_DIRECTION),
    _moving(false),
    _initialized(false),
    _voiceSphereID(GeometryCache::UNKNOWN_ID)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(qApp->thread());

    setScale(glm::vec3(1.0f)); // avatar scale is uniform

    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = static_cast<HeadData*>(new Head(this));

    _skeletonModel = std::make_shared<SkeletonModel>(this, nullptr, rig);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, &Avatar::setModelURLFinished);

    auto geometryCache = DependencyManager::get<GeometryCache>();
    _nameRectGeometryID = geometryCache->allocateID();
    _leftPointerGeometryID = geometryCache->allocateID();
    _rightPointerGeometryID = geometryCache->allocateID();
}

Avatar::~Avatar() {
    assert(isDead()); // mark dead before calling the dtor

    auto treeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (entityTree) {
        entityTree->withWriteLock([&] {
            AvatarEntityMap avatarEntities = getAvatarEntityData();
            for (auto entityID : avatarEntities.keys()) {
                entityTree->deleteEntity(entityID, true, true);
            }
        });
    }

    if (_motionState) {
        delete _motionState;
        _motionState = nullptr;
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


glm::quat Avatar::getWorldAlignedOrientation () const {
    return computeRotationFromBodyToWorldUp() * getOrientation();
}

AABox Avatar::getBounds() const {
    if (!_skeletonModel->isRenderable() || _skeletonModel->needsFixupInScene()) {
        // approximately 2m tall, scaled to user request.
        return AABox(getPosition() - glm::vec3(getUniformScale()), getUniformScale() * 2.0f);
    }
    return _skeletonModel->getRenderableMeshBound();
}

void Avatar::animateScaleChanges(float deltaTime) {
    float currentScale = getUniformScale();
    auto desiredScale = getDomainLimitedScale();
    if (currentScale != desiredScale) {
        // use exponential decay toward the domain limit clamped scale
        const float SCALE_ANIMATION_TIMESCALE = 0.5f;
        float blendFactor = glm::clamp(deltaTime / SCALE_ANIMATION_TIMESCALE, 0.0f, 1.0f);
        float animatedScale = (1.0f - blendFactor) * currentScale + blendFactor * desiredScale;

        // snap to the end when we get close enough
        const float MIN_RELATIVE_SCALE_ERROR = 0.03f;
        if (fabsf(desiredScale - currentScale) / desiredScale < MIN_RELATIVE_SCALE_ERROR) {
            animatedScale = desiredScale;
        }

        setScale(glm::vec3(animatedScale)); // avatar scale is uniform
        rebuildCollisionShape();
    }
}

void Avatar::updateAvatarEntities() {
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

    auto treeRenderer = qApp->getEntities();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    if (!entityTree) {
        return;
    }

    bool success = true;
    QScriptEngine scriptEngine;
    entityTree->withWriteLock([&] {
        AvatarEntityMap avatarEntities = getAvatarEntityData();
        for (auto entityID : avatarEntities.keys()) {
            // see EntityEditPacketSender::queueEditEntityMessage for the other end of this.  unpack properties
            // and either add or update the entity.
            QByteArray jsonByteArray = avatarEntities.value(entityID);
            QJsonDocument jsonProperties = QJsonDocument::fromBinaryData(jsonByteArray);
            if (!jsonProperties.isObject()) {
                qCDebug(interfaceapp) << "got bad avatarEntity json" << QString(jsonByteArray.toHex());
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
                qCDebug(interfaceapp) << "removing entity script from avatar attached entity:" << entityID << "old script:" << attachedScript;
                QString noScript;
                properties.setScript(noScript);
            }

            EntityItemPointer entity = entityTree->findEntityByEntityItemID(EntityItemID(entityID));

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
        }

        AvatarEntityIDs recentlyDettachedAvatarEntities = getAndClearRecentlyDetachedIDs();
        _avatarEntitiesLock.withReadLock([&] {
            foreach (auto entityID, recentlyDettachedAvatarEntities) {
                if (!_avatarEntityData.contains(entityID)) {
                    entityTree->deleteEntity(entityID, true, true);
                }
            }
        });
    });

    if (success) {
        setAvatarEntityDataChanged(false);
    }
}



void Avatar::simulate(float deltaTime) {
    PerformanceTimer perfTimer("simulate");

    if (!isDead() && !_motionState) {
        DependencyManager::get<AvatarManager>()->addAvatarToSimulation(this);
    }
    animateScaleChanges(deltaTime);

    // update the shouldAnimate flag to match whether or not we will render the avatar.
    const float MINIMUM_VISIBILITY_FOR_ON = 0.4f;
    const float MAXIMUM_VISIBILITY_FOR_OFF = 0.6f;
    ViewFrustum viewFrustum;
    qApp->copyViewFrustum(viewFrustum);
    float visibility = calculateRenderAccuracy(viewFrustum.getPosition(),
            getBounds(), DependencyManager::get<LODManager>()->getOctreeSizeScale());
    if (!_shouldAnimate) {
        if (visibility > MINIMUM_VISIBILITY_FOR_ON) {
            _shouldAnimate = true;
            qCDebug(interfaceapp) << "Restoring" << (isMyAvatar() ? "myself" : getSessionUUID()) << "for visibility" << visibility;
        }
    } else if (visibility < MAXIMUM_VISIBILITY_FOR_OFF) {
        _shouldAnimate = false;
        qCDebug(interfaceapp) << "Optimizing" << (isMyAvatar() ? "myself" : getSessionUUID()) << "for visibility" << visibility;
    }

    // simple frustum check
    float boundingRadius = getBoundingRadius();
    qApp->copyDisplayViewFrustum(viewFrustum);
    bool avatarPositionInView = viewFrustum.sphereIntersectsFrustum(getPosition(), boundingRadius);
    bool avatarMeshInView = viewFrustum.boxIntersectsFrustum(_skeletonModel->getRenderableMeshBound());

    if (_shouldAnimate && !_shouldSkipRender && (avatarPositionInView || avatarMeshInView)) {
        {
            PerformanceTimer perfTimer("skeleton");
            _skeletonModel->getRig()->copyJointsFromJointData(_jointData);
            _skeletonModel->simulate(deltaTime, _hasNewJointRotations || _hasNewJointTranslations);
            locationChanged(); // joints changed, so if there are any children, update them.
            _hasNewJointRotations = false;
            _hasNewJointTranslations = false;
        }
        {
            PerformanceTimer perfTimer("head");
            glm::vec3 headPosition = getPosition();
            if (!_skeletonModel->getHeadPosition(headPosition)) {
                headPosition = getPosition();
            }
            Head* head = getHead();
            head->setPosition(headPosition);
            head->setScale(getUniformScale());
            head->simulate(deltaTime, false, !_shouldAnimate);
        }
    } else {
        // a non-full update is still required so that the position, rotation, scale and bounds of the skeletonModel are updated.
        getHead()->setPosition(getPosition());
        _skeletonModel->simulate(deltaTime, false);
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

    measureMotionDerivatives(deltaTime);

    simulateAttachments(deltaTime);
    updatePalms();
    updateAvatarEntities();
}

bool Avatar::isLookingAtMe(AvatarSharedPointer avatar) const {
    const float HEAD_SPHERE_RADIUS = 0.1f;
    glm::vec3 theirLookAt = dynamic_pointer_cast<Avatar>(avatar)->getHead()->getLookAtPosition();
    glm::vec3 myEyePosition = getHead()->getEyePosition();

    return glm::distance(theirLookAt, myEyePosition) <= (HEAD_SPHERE_RADIUS * getUniformScale());
}

void Avatar::slamPosition(const glm::vec3& newPosition) {
    setPosition(newPosition);
    _positionDeltaAccumulator = glm::vec3(0.0f);
    setVelocity(glm::vec3(0.0f));
    _lastVelocity = glm::vec3(0.0f);
}

void Avatar::applyPositionDelta(const glm::vec3& delta) {
    setPosition(getPosition() + delta);
    _positionDeltaAccumulator += delta;
}

void Avatar::measureMotionDerivatives(float deltaTime) {
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

bool Avatar::addToScene(AvatarSharedPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    auto avatarPayload = new render::Payload<AvatarData>(self);
    auto avatarPayloadPointer = Avatar::PayloadPointer(avatarPayload);
    _renderItemID = scene->allocateID();
    pendingChanges.resetItem(_renderItemID, avatarPayloadPointer);
    _skeletonModel->addToScene(scene, pendingChanges);

    for (auto& attachmentModel : _attachmentModels) {
        attachmentModel->addToScene(scene, pendingChanges);
    }

    return true;
}

void Avatar::removeFromScene(AvatarSharedPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_renderItemID);
    render::Item::clearID(_renderItemID);
    _skeletonModel->removeFromScene(scene, pendingChanges);
    for (auto& attachmentModel : _attachmentModels) {
        attachmentModel->removeFromScene(scene, pendingChanges);
    }
}

void Avatar::updateRenderItem(render::PendingChanges& pendingChanges) {
    if (render::Item::isValidID(_renderItemID)) {
        pendingChanges.updateItem<render::Payload<AvatarData>>(_renderItemID, [](render::Payload<AvatarData>& p) {});
    }
}

void Avatar::postUpdate(float deltaTime) {

    bool renderLookAtVectors;
    if (isMyAvatar()) {
        renderLookAtVectors = Menu::getInstance()->isOptionChecked(MenuOption::RenderMyLookAtVectors);
    } else {
        renderLookAtVectors = Menu::getInstance()->isOptionChecked(MenuOption::RenderOtherLookAtVectors);
    }

    if (renderLookAtVectors) {
        const float EYE_RAY_LENGTH = 10.0;
        const glm::vec4 BLUE(0.0f, 0.0f, 1.0f, 1.0f);
        const glm::vec4 RED(1.0f, 0.0f, 0.0f, 1.0f);

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
}

void Avatar::render(RenderArgs* renderArgs, const glm::vec3& cameraPosition) {
    auto& batch = *renderArgs->_batch;
    PROFILE_RANGE_BATCH(batch, __FUNCTION__);

    if (glm::distance(DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition(), getPosition()) < 10.0f) {
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

    { // simple frustum check
        ViewFrustum frustum;
        if (renderArgs->_renderMode == RenderArgs::SHADOW_RENDER_MODE) {
            qApp->copyShadowViewFrustum(frustum);
        } else {
            qApp->copyDisplayViewFrustum(frustum);
        }
        if (!frustum.sphereIntersectsFrustum(getPosition(), getBoundingRadius())) {
            return;
        }
    }

    glm::vec3 toTarget = cameraPosition - getPosition();
    float distanceToTarget = glm::length(toTarget);

    {
        fixupModelsInScene();

        if (renderArgs->_renderMode != RenderArgs::SHADOW_RENDER_MODE) {
            // add local lights
            const float BASE_LIGHT_DISTANCE = 2.0f;
            const float LIGHT_FALLOFF_RADIUS = 0.01f;
            const float LIGHT_EXPONENT = 1.0f;
            const float LIGHT_CUTOFF = glm::radians(80.0f);
            float distance = BASE_LIGHT_DISTANCE * getUniformScale();
            glm::vec3 position = _skeletonModel->getTranslation();
            glm::quat orientation = getOrientation();
            foreach (const AvatarManager::LocalLight& light, DependencyManager::get<AvatarManager>()->getLocalLights()) {
                glm::vec3 direction = orientation * light.direction;
                DependencyManager::get<DeferredLightingEffect>()->addSpotLight(position - direction * distance,
                    distance * 2.0f, light.color, 0.5f, LIGHT_FALLOFF_RADIUS, orientation, LIGHT_EXPONENT, LIGHT_CUTOFF);
            }
        }

        bool renderBounding = Menu::getInstance()->isOptionChecked(MenuOption::RenderBoundingCollisionShapes);
        if (renderBounding && shouldRenderHead(renderArgs) && _skeletonModel->isRenderable()) {
            PROFILE_RANGE_BATCH(batch, __FUNCTION__":skeletonBoundingCollisionShapes");
            const float BOUNDING_SHAPE_ALPHA = 0.7f;
            _skeletonModel->renderBoundingCollisionShapes(*renderArgs->_batch, getUniformScale(), BOUNDING_SHAPE_ALPHA);
        }
    }

    const float DISPLAYNAME_DISTANCE = 20.0f;
    setShowDisplayName(distanceToTarget < DISPLAYNAME_DISTANCE);

    auto cameraMode = qApp->getCamera()->getMode();
    if (!isMyAvatar() || cameraMode != CAMERA_MODE_FIRST_PERSON) {
        auto& frustum = renderArgs->getViewFrustum();
        auto textPosition = getDisplayNamePosition();
        if (frustum.pointIntersectsFrustum(textPosition)) {
            renderDisplayName(batch, frustum, textPosition);
        }
    }
}

glm::quat Avatar::computeRotationFromBodyToWorldUp(float proportion) const {
    glm::quat orientation = getOrientation();
    glm::vec3 currentUp = orientation * IDENTITY_UP;
    float angle = acosf(glm::clamp(glm::dot(currentUp, _worldUpDirection), -1.0f, 1.0f));
    if (angle < EPSILON) {
        return glm::quat();
    }
    glm::vec3 axis;
    if (angle > 179.99f * RADIANS_PER_DEGREE) { // 180 degree rotation; must use another axis
        axis = orientation * IDENTITY_RIGHT;
    } else {
        axis = glm::normalize(glm::cross(currentUp, _worldUpDirection));
    }
    return glm::angleAxis(angle * proportion, axis);
}

void Avatar::fixupModelsInScene() {
    _attachmentsToDelete.clear();

    // check to see if when we added our models to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::ScenePointer scene = qApp->getMain3DScene();
    render::PendingChanges pendingChanges;
    if (_skeletonModel->isRenderable() && _skeletonModel->needsFixupInScene()) {
        _skeletonModel->removeFromScene(scene, pendingChanges);
        _skeletonModel->addToScene(scene, pendingChanges);
    }
    for (auto attachmentModel : _attachmentModels) {
        if (attachmentModel->isRenderable() && attachmentModel->needsFixupInScene()) {
            attachmentModel->removeFromScene(scene, pendingChanges);
            attachmentModel->addToScene(scene, pendingChanges);
        }
    }

    for (auto attachmentModelToRemove : _attachmentsToRemove) {
        attachmentModelToRemove->removeFromScene(scene, pendingChanges);
    }
    _attachmentsToDelete.insert(_attachmentsToDelete.end(), _attachmentsToRemove.begin(), _attachmentsToRemove.end());
    _attachmentsToRemove.clear();
    scene->enqueuePendingChanges(pendingChanges);
}

bool Avatar::shouldRenderHead(const RenderArgs* renderArgs) const {
    return true;
}

// virtual
void Avatar::simulateAttachments(float deltaTime) {
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
                model->setTranslation(jointPosition + jointRotation * attachment.translation * getUniformScale());
                model->setRotation(jointRotation * attachment.rotation);
                model->setScaleToFit(true, getUniformScale() * attachment.scale, true); // hack to force rescale
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
        qCWarning(interfaceapp) << "debugValue() " << str << value;
    }
};
void debugValue(const QString& str, const float& value) {
    if (glm::isnan(value) || glm::isinf(value)) {
        qCWarning(interfaceapp) << "debugValue() " << str << value;
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

    if (getSkeletonModel()->getNeckPosition(namePosition)) {
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
        qCWarning(interfaceapp) << "Invalid display name position" << namePosition
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

    bool shouldShowReceiveStats = DependencyManager::get<AvatarManager>()->shouldShowReceiveStats() && !isMyAvatar();

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
    const float MAX_OFFSET_LENGTH = getUniformScale() * 0.5f;
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

glm::quat Avatar::getAbsoluteJointRotationInObjectFrame(int index) const {
    switch(index) {
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            bool success;
            Transform avatarTransform;
            Transform::mult(avatarTransform, getParentTransform(success), getLocalTransform());
            glm::mat4 invAvatarMat = avatarTransform.getInverseMatrix();
            return glmExtractRotation(invAvatarMat * sensorToWorldMatrix);
        }
        case CONTROLLER_LEFTHAND_INDEX: {
            Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
            return controllerLeftHandTransform.getRotation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
            return controllerRightHandTransform.getRotation();
        }
        default: {
            glm::quat rotation;
            _skeletonModel->getAbsoluteJointRotationInRigFrame(index, rotation);
            return Quaternions::Y_180 * rotation;
        }
    }
}

glm::vec3 Avatar::getAbsoluteJointTranslationInObjectFrame(int index) const {
    switch(index) {
        case SENSOR_TO_WORLD_MATRIX_INDEX: {
            glm::mat4 sensorToWorldMatrix = getSensorToWorldMatrix();
            bool success;
            Transform avatarTransform;
            Transform::mult(avatarTransform, getParentTransform(success), getLocalTransform());
            glm::mat4 invAvatarMat = avatarTransform.getInverseMatrix();
            return extractTranslation(invAvatarMat * sensorToWorldMatrix);
        }
        case CONTROLLER_LEFTHAND_INDEX: {
            Transform controllerLeftHandTransform = Transform(getControllerLeftHandMatrix());
            return controllerLeftHandTransform.getTranslation();
        }
        case CONTROLLER_RIGHTHAND_INDEX: {
            Transform controllerRightHandTransform = Transform(getControllerRightHandMatrix());
            return controllerRightHandTransform.getTranslation();
        }
        default: {
            glm::vec3 translation;
            _skeletonModel->getAbsoluteJointTranslationInRigFrame(index, translation);
            return Quaternions::Y_180 * translation;
        }
    }
}

int Avatar::getJointIndex(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        int result;
        QMetaObject::invokeMethod(const_cast<Avatar*>(this), "getJointIndex", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(int, result), Q_ARG(const QString&, name));
        return result;
    }
    int result = getFauxJointIndex(name);
    if (result != -1) {
        return result;
    }
    return _skeletonModel->isActive() ? _skeletonModel->getFBXGeometry().getJointIndex(name) : -1;
}

QStringList Avatar::getJointNames() const {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        QMetaObject::invokeMethod(const_cast<Avatar*>(this), "getJointNames", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QStringList, result));
        return result;
    }
    return _skeletonModel->isActive() ? _skeletonModel->getFBXGeometry().getJointNames() : QStringList();
}

glm::vec3 Avatar::getJointPosition(int index) const {
    if (QThread::currentThread() != thread()) {
        glm::vec3 position;
        QMetaObject::invokeMethod(const_cast<Avatar*>(this), "getJointPosition", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(glm::vec3, position), Q_ARG(const int, index));
        return position;
    }
    glm::vec3 position;
    _skeletonModel->getJointPositionInWorldFrame(index, position);
    return position;
}

glm::vec3 Avatar::getJointPosition(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        glm::vec3 position;
        QMetaObject::invokeMethod(const_cast<Avatar*>(this), "getJointPosition", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(glm::vec3, position), Q_ARG(const QString&, name));
        return position;
    }
    glm::vec3 position;
    _skeletonModel->getJointPositionInWorldFrame(getJointIndex(name), position);
    return position;
}

void Avatar::scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const {
    //Scale a world space vector as if it was relative to the position
    positionToScale = getPosition() + getUniformScale() * (positionToScale - getPosition());
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
    if (!success && _skeletonModelURL != AvatarData::defaultFullAvatarModelUrl()) {
        qDebug() << "Using default after failing to load Avatar model: " << _skeletonModelURL;
        // call _skeletonModel.setURL, but leave our copy of _skeletonModelURL alone.  This is so that
        // we don't redo this every time we receive an identity packet from the avatar with the bad url.
        QMetaObject::invokeMethod(_skeletonModel.get(), "setURL",
                                  Qt::QueuedConnection, Q_ARG(QUrl, AvatarData::defaultFullAvatarModelUrl()));
    }
}


// create new model, can return an instance of a SoftAttachmentModel rather then Model
static std::shared_ptr<Model> allocateAttachmentModel(bool isSoft, RigPointer rigOverride) {
    if (isSoft) {
        // cast to std::shared_ptr<Model>
        return std::dynamic_pointer_cast<Model>(std::make_shared<SoftAttachmentModel>(std::make_shared<Rig>(), nullptr, rigOverride));
    } else {
        return std::make_shared<Model>(std::make_shared<Rig>());
    }
}

void Avatar::setAttachmentData(const QVector<AttachmentData>& attachmentData) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setAttachmentData", Qt::BlockingQueuedConnection,
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
            _attachmentModels.push_back(allocateAttachmentModel(attachmentData[i].isSoft, _skeletonModel->getRig()));
        }
        else if (i < oldAttachmentData.size() && oldAttachmentData[i].isSoft != attachmentData[i].isSoft) {
            // if the attachment has changed type, we need to re-allocate a new one.
            _attachmentsToRemove.push_back(_attachmentModels[i]);
            _attachmentModels[i] = allocateAttachmentModel(attachmentData[i].isSoft, _skeletonModel->getRig());
        }
        _attachmentModels[i]->setURL(attachmentData[i].modelURL);
    }
}


int Avatar::parseDataFromBuffer(const QByteArray& buffer) {
    if (!_initialized) {
        // now that we have data for this Avatar we are go for init
        init();
    }

    // change in position implies movement
    glm::vec3 oldPosition = getPosition();

    int bytesRead = AvatarData::parseDataFromBuffer(buffer);

    const float MOVE_DISTANCE_THRESHOLD = 0.001f;
    _moving = glm::distance(oldPosition, getPosition()) > MOVE_DISTANCE_THRESHOLD;
    if (_moving && _motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_POSITION);
    }
    if (_moving || _hasNewJointRotations || _hasNewJointTranslations) {
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

void Avatar::setShowDisplayName(bool showDisplayName) {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::NamesAboveHeads)) {
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
    float uniformScale = getUniformScale();
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

void Avatar::setMotionState(AvatarMotionState* motionState) {
    _motionState = motionState;
}

// virtual
void Avatar::rebuildCollisionShape() {
    if (_motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_SHAPE);
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
    getSkeletonModel()->getJointRotationInWorldFrame(getSkeletonModel()->getLeftHandJointIndex(), leftPalmRotation);
    getSkeletonModel()->getLeftHandPosition(leftPalmPosition);
    leftPalmPosition += HAND_TO_PALM_OFFSET * glm::inverse(leftPalmRotation);
    return leftPalmPosition;
}

glm::quat Avatar::getUncachedLeftPalmRotation() const {
    assert(QThread::currentThread() == thread());  // main thread access only
    glm::quat leftPalmRotation;
    getSkeletonModel()->getJointRotationInWorldFrame(getSkeletonModel()->getLeftHandJointIndex(), leftPalmRotation);
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
    getSkeletonModel()->getJointRotationInWorldFrame(getSkeletonModel()->getRightHandJointIndex(), rightPalmRotation);
    getSkeletonModel()->getRightHandPosition(rightPalmPosition);
    rightPalmPosition += HAND_TO_PALM_OFFSET * glm::inverse(rightPalmRotation);
    return rightPalmPosition;
}

glm::quat Avatar::getUncachedRightPalmRotation() const {
    assert(QThread::currentThread() == thread());  // main thread access only
    glm::quat rightPalmRotation;
    getSkeletonModel()->getJointRotationInWorldFrame(getSkeletonModel()->getRightHandJointIndex(), rightPalmRotation);
    return rightPalmRotation;
}

void Avatar::setPosition(const glm::vec3& position) {
    AvatarData::setPosition(position);
    updateAttitude();
}

void Avatar::setOrientation(const glm::quat& orientation) {
    AvatarData::setOrientation(orientation);
    updateAttitude();
}

void Avatar::updatePalms() {
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
    bool success;
    Transform beforeChangeTransform = getTransform(success);
    SpatiallyNestable::setParentID(parentID);
    if (success) {
        setTransform(beforeChangeTransform, success);
        if (!success) {
            qCDebug(interfaceapp) << "Avatar::setParentID failed to reset avatar's location.";
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
            qCDebug(interfaceapp) << "Avatar::setParentJointIndex failed to reset avatar's location.";
        }
    }
}
