//
//  SkeletonModel.cpp
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SkeletonModel.h"

#include <glm/gtx/transform.hpp>
#include <QMultiMap>

#include <recording/Deck.h>
#include <DebugDraw.h>
#include <AnimDebugDraw.h>
#include <CharacterController.h>

#include "Avatar.h"
#include "Logging.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar, QObject* parent) :
    CauterizedModel(parent),
    _owningAvatar(owningAvatar),
    _boundingCapsuleLocalOffset(0.0f),
    _boundingCapsuleRadius(0.0f),
    _boundingCapsuleHeight(0.0f),
    _defaultEyeModelPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
    _headClipDistance(DEFAULT_NEAR_CLIP)
{
    // SkeletonModels, and by extention Avatars, use Dual Quaternion skinning.
    _useDualQuaternionSkinning = true;

    // Avatars all cast shadow
    setCanCastShadow(true);

    assert(_owningAvatar);
}

SkeletonModel::~SkeletonModel() {
}

void SkeletonModel::setURL(const QUrl& url) {
    _texturesLoaded = false;
    Model::setURL(url);
}

void SkeletonModel::setTextures(const QVariantMap& textures) {
    _texturesLoaded = false;
    Model::setTextures(textures);
}

void SkeletonModel::initJointStates() {
    const HFMModel& hfmModel = getHFMModel();
    glm::mat4 modelOffset = glm::scale(_scale) * glm::translate(_offset);
    _rig.initJointStates(hfmModel, modelOffset);

    {
        // initialize _jointData with proper values for default joints
        QVector<JointData> defaultJointData;
        _rig.copyJointsIntoJointData(defaultJointData);
        _owningAvatar->setRawJointData(defaultJointData);
    }

    // Determine the default eye position for avatar scale = 1.0
    int headJointIndex = _rig.indexOfJoint("Head");
    if (0 > headJointIndex || headJointIndex >= _rig.getJointStateCount()) {
        qCWarning(avatars_renderer) << "Bad head joint! Got:" << headJointIndex << "jointCount:" << _rig.getJointStateCount();
    }
    glm::vec3 leftEyePosition, rightEyePosition;
    getEyeModelPositions(leftEyePosition, rightEyePosition);
    glm::vec3 midEyePosition = (leftEyePosition + rightEyePosition) / 2.0f;

    int rootJointIndex = _rig.indexOfJoint("Hips");
    glm::vec3 rootModelPosition;
    getJointPosition(rootJointIndex, rootModelPosition);

    _defaultEyeModelPosition = midEyePosition - rootModelPosition;

    // Skeleton may have already been scaled so unscale it
    _defaultEyeModelPosition = _defaultEyeModelPosition / _scale;

    computeBoundingShape();

    Extents meshExtents = getMeshExtents();
    _headClipDistance = -(meshExtents.minimum.z / _scale.z - _defaultEyeModelPosition.z);
    _headClipDistance = std::max(_headClipDistance, DEFAULT_NEAR_CLIP);

    _owningAvatar->rebuildCollisionShape();
    emit skeletonLoaded();
}

glm::vec3 SkeletonModel::avoidCrossedEyes(const glm::vec3& lookAt) {
    // make sure lookAt is not too close to face (avoid crosseyes)
    glm::vec3 focusOffset = lookAt - _owningAvatar->getHead()->getEyePosition();
    float focusDistance = glm::length(focusOffset);
    const float MIN_LOOK_AT_FOCUS_DISTANCE = 1.0f;
    if (focusDistance < MIN_LOOK_AT_FOCUS_DISTANCE && focusDistance > EPSILON) {
        return _owningAvatar->getHead()->getEyePosition() + (MIN_LOOK_AT_FOCUS_DISTANCE / focusDistance) * focusOffset;
    } else {
        return lookAt;
    }
}

// Called within Model::simulate call, below.
void SkeletonModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    assert(!_owningAvatar->isMyAvatar());

    Head* head = _owningAvatar->getHead();
    glm::vec3 lookAt = avoidCrossedEyes(head->getCorrectedLookAtPosition());

    // no need to call Model::updateRig() because otherAvatars get their joint state
    // copied directly from AvtarData::_jointData (there are no Rig animations to blend)
    _needsUpdateClusterMatrices = true;

    // This is a little more work than we really want.
    //
    // Other avatars joint, including their eyes, should already be set just like any other joints
    // from the wire data. But when looking at me, we want the eyes to use the corrected lookAt.
    //
    // Thus this should really only be ... else if (_owningAvatar->getHead()->isLookingAtMe()) {...
    // However, in the !isLookingAtMe case, the eyes aren't rotating the way they should right now.
    // We will revisit that as priorities allow, and particularly after the new rig/animation/joints.

    // If the head is not positioned, updateEyeJoints won't get the math right
    glm::quat headOrientation;
    _rig.getJointRotation(_rig.indexOfJoint("Head"), headOrientation);
    glm::vec3 eulers = safeEulerAngles(headOrientation);
    head->setBasePitch(glm::degrees(-eulers.x));
    head->setBaseYaw(glm::degrees(eulers.y));
    head->setBaseRoll(glm::degrees(-eulers.z));

    Rig::EyeParameters eyeParams;
    eyeParams.eyeLookAt = lookAt;
    eyeParams.eyeSaccade = glm::vec3(0.0f);
    eyeParams.modelRotation = getRotation();
    eyeParams.modelTranslation = getTranslation();
    eyeParams.leftEyeJointIndex = _rig.indexOfJoint("LeftEye");
    eyeParams.rightEyeJointIndex = _rig.indexOfJoint("RightEye");

    _rig.updateFromEyeParameters(eyeParams);
}

void SkeletonModel::updateAttitude(const glm::quat& orientation) {
    setTranslation(_owningAvatar->getSkeletonPosition());
    setRotation(orientation * Quaternions::Y_180);
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningAvatar->getModelScale());
}

// Called by Avatar::simulate after it has set the joint states (fullUpdate true if changed),
// but just before head has been simulated.
void SkeletonModel::simulate(float deltaTime, bool fullUpdate) {
    updateAttitude(_owningAvatar->getWorldOrientation());
    setBlendshapeCoefficients(_owningAvatar->getHead()->getSummedBlendshapeCoefficients());

    Parent::simulate(deltaTime, fullUpdate);
    if (fullUpdate) {
        // let rig compute the model offset
        glm::vec3 registrationPoint;
        if (_rig.getModelRegistrationPoint(registrationPoint)) {
            setOffset(registrationPoint);
        }
    }

    // FIXME: This texture loading logic should probably live in Avatar, to mirror RenderableModelEntityItem,
    // but Avatars don't get updates in the same way
    if (!_texturesLoaded && getGeometry() && getGeometry()->areTexturesLoaded()) {
        _texturesLoaded = true;
        updateRenderItems();
    }

    if (!isLoaded() || !_owningAvatar->isMyAvatar()) {
        return; // only simulate for own avatar
    }

    auto player = DependencyManager::get<recording::Deck>();
    if (player->isPlaying()) {
        return;
    }
}

class IndexValue {
public:
    int index;
    float value;
};

bool operator<(const IndexValue& firstIndex, const IndexValue& secondIndex) {
    return firstIndex.value < secondIndex.value;
}

bool SkeletonModel::getLeftGrabPosition(glm::vec3& position) const {
    int knuckleIndex = _rig.indexOfJoint("LeftHandMiddle1");
    int handIndex = _rig.indexOfJoint("LeftHand");
    if (knuckleIndex >= 0 && handIndex >= 0) {
        glm::quat handRotation;
        glm::vec3 knucklePosition;
        glm::vec3 handPosition;
        if (!getJointPositionInWorldFrame(knuckleIndex, knucklePosition)) {
            return false;
        }
        if (!getJointPositionInWorldFrame(handIndex, handPosition)) {
            return false;
        }
        if (!getJointRotationInWorldFrame(handIndex, handRotation)) {
            return false;
        }
        float halfPalmLength = glm::distance(knucklePosition, handPosition) * 0.5f;
        // z azis is standardized to be out of the palm.  move from the knuckle-joint away from the palm
        // by 1/2 the palm length.
        position = knucklePosition + handRotation * (glm::vec3(0.0f, 0.0f, 1.0f) * halfPalmLength);
        return true;
    }
    return false;
}

bool SkeletonModel::getRightGrabPosition(glm::vec3& position) const {
    int knuckleIndex = _rig.indexOfJoint("RightHandMiddle1");
    int handIndex = _rig.indexOfJoint("RightHand");
    if (knuckleIndex >= 0 && handIndex >= 0) {
        glm::quat handRotation;
        glm::vec3 knucklePosition;
        glm::vec3 handPosition;
        if (!getJointPositionInWorldFrame(knuckleIndex, knucklePosition)) {
            return false;
        }
        if (!getJointPositionInWorldFrame(handIndex, handPosition)) {
            return false;
        }
        if (!getJointRotationInWorldFrame(handIndex, handRotation)) {
            return false;
        }
        float halfPalmLength = glm::distance(knucklePosition, handPosition) * 0.5f;
        // z azis is standardized to be out of the palm.  move from the knuckle-joint away from the palm
        // by 1/2 the palm length.
        position = knucklePosition + handRotation * (glm::vec3(0.0f, 0.0f, 1.0f) * halfPalmLength);
        return true;
    }
    return false;
}

bool SkeletonModel::getLeftHandPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getLeftHandJointIndex(), position);
}

bool SkeletonModel::getRightHandPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getRightHandJointIndex(), position);
}

bool SkeletonModel::getHeadPosition(glm::vec3& headPosition) const {
    return isLoaded() && getJointPositionInWorldFrame(_rig.indexOfJoint("Head"), headPosition);
}

bool SkeletonModel::getNeckPosition(glm::vec3& neckPosition) const {
    return isLoaded() && getJointPositionInWorldFrame(_rig.indexOfJoint("Neck"), neckPosition);
}

bool SkeletonModel::getLocalNeckPosition(glm::vec3& neckPosition) const {
    return isLoaded() && getJointPosition(_rig.indexOfJoint("Neck"), neckPosition);
}

bool SkeletonModel::getEyeModelPositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isLoaded()) {
        return false;
    }

    if (getJointPosition(_rig.indexOfJoint("LeftEye"), firstEyePosition) &&
        getJointPosition(_rig.indexOfJoint("RightEye"), secondEyePosition)) {
        return true;
    }

    int headJointIndex = _rig.indexOfJoint("Head");
    glm::vec3 headPosition;
    if (getJointPosition(headJointIndex, headPosition)) {

        // get head joint rotation.
        glm::quat headRotation;
        getJointRotation(headJointIndex, headRotation);

        float heightRatio = _rig.getUnscaledEyeHeight() / DEFAULT_AVATAR_EYE_HEIGHT;
        glm::vec3 ipdOffset = glm::vec3(DEFAULT_AVATAR_IPD / 2.0f, 0.0f, 0.0f);
        firstEyePosition = headPosition + headRotation * heightRatio * (DEFAULT_AVATAR_HEAD_TO_MIDDLE_EYE_OFFSET + ipdOffset);
        secondEyePosition = headPosition + headRotation * heightRatio * (DEFAULT_AVATAR_HEAD_TO_MIDDLE_EYE_OFFSET - ipdOffset);
        return true;
    }
    return false;
}


bool SkeletonModel::getIsJointOverridden(int jointIndex) const {
    // has this joint been set by a script?
    if (!isLoaded() || _rig.jointStatesEmpty()) {
        return false;
    }
    return _rig.getIsJointOverridden(jointIndex);
}

bool SkeletonModel::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (getEyeModelPositions(firstEyePosition, secondEyePosition)) {
        firstEyePosition = _translation + _rotation * firstEyePosition;
        secondEyePosition = _translation + _rotation * secondEyePosition;
        return true;
    }
    return false;
}

glm::vec3 SkeletonModel::getDefaultEyeModelPosition() const {
    return _owningAvatar->getModelScale() * _defaultEyeModelPosition;
}

float DENSITY_OF_WATER = 1000.0f; // kg/m^3
float MIN_JOINT_MASS = 1.0f;
float VERY_BIG_MASS = 1.0e6f;

// virtual
void SkeletonModel::computeBoundingShape() {
    if (!isLoaded() || _rig.jointStatesEmpty()) {
        return;
    }

    const HFMModel& hfmModel = getHFMModel();
    if (hfmModel.joints.isEmpty() || _rig.indexOfJoint("Hips") == -1) {
        // rootJointIndex == -1 if the avatar model has no skeleton
        return;
    }

    float radius, height;
    glm::vec3 offset;
    _rig.computeAvatarBoundingCapsule(hfmModel, radius, height, offset);
    float invScale = 1.0f / _owningAvatar->getModelScale();
    _boundingCapsuleRadius = invScale * radius;
    _boundingCapsuleHeight = invScale * height;
    _boundingCapsuleLocalOffset = invScale * offset;
}

void SkeletonModel::renderBoundingCollisionShapes(RenderArgs* args, gpu::Batch& batch, float scale, float alpha) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    // draw a blue sphere at the capsule top point
    glm::vec3 topPoint = _translation + _rotation * (scale * (_boundingCapsuleLocalOffset + (0.5f * _boundingCapsuleHeight) * Vectors::UNIT_Y));
    batch.setModelTransform(Transform().setTranslation(topPoint).postScale(scale * _boundingCapsuleRadius));
    auto pipeline = geometryCache->getShapePipelinePointer(alpha < 1.0f, false, args->_renderMethod == render::Args::RenderMethod::FORWARD);
    geometryCache->renderSolidSphereInstance(args, batch, glm::vec4(0.6f, 0.6f, 0.8f, alpha), pipeline);

    // draw a yellow sphere at the capsule bottom point
    glm::vec3 bottomPoint = topPoint - _rotation * glm::vec3(0.0f, scale * _boundingCapsuleHeight, 0.0f);
    batch.setModelTransform(Transform().setTranslation(bottomPoint).postScale(scale * _boundingCapsuleRadius));
    geometryCache->renderSolidSphereInstance(args, batch, glm::vec4(0.8f, 0.8f, 0.6f, alpha), pipeline);

    // draw a green cylinder between the two points
    float capsuleDiameter = 2.0f * _boundingCapsuleRadius;
    glm::vec3 cylinderDimensions = glm::vec3(capsuleDiameter, _boundingCapsuleHeight, capsuleDiameter);
    batch.setModelTransform(Transform().setScale(scale * cylinderDimensions).setRotation(_rotation).setTranslation(0.5f * (topPoint + bottomPoint)));
    geometryCache->renderSolidShapeInstance(args, batch, GeometryCache::Shape::Cylinder, glm::vec4(0.6f, 0.8f, 0.6f, alpha), pipeline);
}

bool SkeletonModel::hasSkeleton() {
    return isLoaded() ? _rig.indexOfJoint("Hips") != -1 : false;
}

void SkeletonModel::onInvalidate() {
}
