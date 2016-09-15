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

#include <glm/gtx/transform.hpp>
#include <QMultiMap>

#include <recording/Deck.h>
#include <DebugDraw.h>

#include "Application.h"
#include "Avatar.h"
#include "Menu.h"
#include "SkeletonModel.h"
#include "Util.h"
#include "InterfaceLogging.h"
#include "AnimDebugDraw.h"

glm::vec3 TRUNCATE_IK_CAPSULE_POSITION(0.0f, 0.0f, 0.0f);
float TRUNCATE_IK_CAPSULE_LENGTH = 1000.0f;
float TRUNCATE_IK_CAPSULE_RADIUS = 0.25f;

SkeletonModel::SkeletonModel(Avatar* owningAvatar, QObject* parent, RigPointer rig) :
    Model(rig, parent),
    _owningAvatar(owningAvatar),
    _boundingCapsuleLocalOffset(0.0f),
    _boundingCapsuleRadius(0.0f),
    _boundingCapsuleHeight(0.0f),
    _defaultEyeModelPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
    _headClipDistance(DEFAULT_NEAR_CLIP)
{
    assert(_rig);
    assert(_owningAvatar);
}

SkeletonModel::~SkeletonModel() {
}

void SkeletonModel::initJointStates() {
    const FBXGeometry& geometry = getFBXGeometry();
    glm::mat4 modelOffset = glm::scale(_scale) * glm::translate(_offset);
    _rig->initJointStates(geometry, modelOffset);

    // Determine the default eye position for avatar scale = 1.0
    int headJointIndex = geometry.headJointIndex;
    if (0 > headJointIndex || headJointIndex >= _rig->getJointStateCount()) {
        qCWarning(interfaceapp) << "Bad head joint! Got:" << headJointIndex << "jointCount:" << _rig->getJointStateCount();
    }
    glm::vec3 leftEyePosition, rightEyePosition;
    getEyeModelPositions(leftEyePosition, rightEyePosition);
    glm::vec3 midEyePosition = (leftEyePosition + rightEyePosition) / 2.0f;

    int rootJointIndex = geometry.rootJointIndex;
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

Rig::CharacterControllerState convertCharacterControllerState(CharacterController::State state) {
    switch (state) {
    default:
    case CharacterController::State::Ground:
        return Rig::CharacterControllerState::Ground;
    case CharacterController::State::Takeoff:
        return Rig::CharacterControllerState::Takeoff;
    case CharacterController::State::InAir:
        return Rig::CharacterControllerState::InAir;
    case CharacterController::State::Hover:
        return Rig::CharacterControllerState::Hover;
    };
}

// Called within Model::simulate call, below.
void SkeletonModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    const FBXGeometry& geometry = getFBXGeometry();

    Head* head = _owningAvatar->getHead();

    // make sure lookAt is not too close to face (avoid crosseyes)
    glm::vec3 lookAt = _owningAvatar->isMyAvatar() ?  head->getLookAtPosition() : head->getCorrectedLookAtPosition();
    glm::vec3 focusOffset = lookAt - _owningAvatar->getHead()->getEyePosition();
    float focusDistance = glm::length(focusOffset);
    const float MIN_LOOK_AT_FOCUS_DISTANCE = 1.0f;
    if (focusDistance < MIN_LOOK_AT_FOCUS_DISTANCE && focusDistance > EPSILON) {
        lookAt = _owningAvatar->getHead()->getEyePosition() + (MIN_LOOK_AT_FOCUS_DISTANCE / focusDistance) * focusOffset;
    }

    if (_owningAvatar->isMyAvatar()) {
        MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);

        Rig::HeadParameters headParams;

        glm::vec3 hmdPositionInRigSpace;
        glm::vec3 truncatedHMDPositionInRigSpace;

        if (qApp->isHMDMode()) {
            headParams.isInHMD = true;

            // get HMD position from sensor space into world space, and back into rig space
            glm::mat4 worldHMDMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
            glm::mat4 rigToWorld = createMatFromQuatAndPos(getRotation(), getTranslation());
            glm::mat4 worldToRig = glm::inverse(rigToWorld);
            glm::mat4 rigHMDMat = worldToRig * worldHMDMat;

            hmdPositionInRigSpace = extractTranslation(rigHMDMat);

            // truncate head IK target if it's out of body
            if (myAvatar->isOutOfBody()) {
                truncatedHMDPositionInRigSpace = projectPointOntoCapsule(hmdPositionInRigSpace, TRUNCATE_IK_CAPSULE_POSITION,
                                                                         TRUNCATE_IK_CAPSULE_LENGTH, TRUNCATE_IK_CAPSULE_RADIUS);
            } else {
                truncatedHMDPositionInRigSpace = hmdPositionInRigSpace;
            }

            headParams.rigHeadPosition = truncatedHMDPositionInRigSpace;
            headParams.rigHeadOrientation = extractRotation(rigHMDMat);
            headParams.worldHeadOrientation = extractRotation(worldHMDMat);

        } else {
            headParams.isInHMD = false;

            // We don't have a valid localHeadPosition.
            headParams.rigHeadOrientation = Quaternions::Y_180 * head->getFinalOrientationInLocalFrame();
            headParams.worldHeadOrientation = head->getFinalOrientationInWorldFrame();
        }

        headParams.neckJointIndex = geometry.neckJointIndex;
        headParams.isTalking = head->getTimeWithoutTalking() <= 1.5f;

        _rig->updateFromHeadParameters(headParams, deltaTime);

        Rig::HandParameters handParams;

        auto leftPose = myAvatar->getLeftHandControllerPoseInAvatarFrame();
        if (leftPose.isValid()) {
            handParams.isLeftEnabled = true;
            handParams.leftPosition = Quaternions::Y_180 * leftPose.getTranslation();
            handParams.leftOrientation = Quaternions::Y_180 * leftPose.getRotation();

            // truncate hand target
            if (myAvatar->isOutOfBody() && qApp->isHMDMode()) {
                glm::vec3 offset = handParams.leftPosition - hmdPositionInRigSpace;
                handParams.leftPosition = truncatedHMDPositionInRigSpace + offset;
            }
        } else {
            handParams.isLeftEnabled = false;
        }

        auto rightPose = myAvatar->getRightHandControllerPoseInAvatarFrame();
        if (rightPose.isValid()) {
            handParams.isRightEnabled = true;
            handParams.rightPosition = Quaternions::Y_180 * rightPose.getTranslation();
            handParams.rightOrientation = Quaternions::Y_180 * rightPose.getRotation();

            // truncate hand target
            if (myAvatar->isOutOfBody() && qApp->isHMDMode()) {
                glm::vec3 offset = handParams.rightPosition - hmdPositionInRigSpace;
                handParams.rightPosition = truncatedHMDPositionInRigSpace + offset;
            }
        } else {
            handParams.isRightEnabled = false;
        }

        handParams.bodyCapsuleRadius = myAvatar->getCharacterController()->getCapsuleRadius();
        handParams.bodyCapsuleHalfHeight = myAvatar->getCharacterController()->getCapsuleHalfHeight();
        handParams.bodyCapsuleLocalOffset = myAvatar->getCharacterController()->getCapsuleLocalOffset();

        _rig->updateFromHandParameters(handParams, deltaTime);

        Rig::CharacterControllerState ccState = convertCharacterControllerState(myAvatar->getCharacterController()->getState());

        auto velocity = myAvatar->getLocalVelocity();
        auto position = myAvatar->getLocalPosition();
        auto orientation = myAvatar->getLocalOrientation();
        _rig->computeMotionAnimationState(deltaTime, position, velocity, orientation, ccState);

        // evaluate AnimGraph animation and update jointStates.
        Model::updateRig(deltaTime, parentTransform);

        Rig::EyeParameters eyeParams;
        eyeParams.worldHeadOrientation = headParams.worldHeadOrientation;
        eyeParams.eyeLookAt = lookAt;
        eyeParams.eyeSaccade = head->getSaccade();
        eyeParams.modelRotation = getRotation();
        eyeParams.modelTranslation = getTranslation();
        eyeParams.leftEyeJointIndex = geometry.leftEyeJointIndex;
        eyeParams.rightEyeJointIndex = geometry.rightEyeJointIndex;

        _rig->updateFromEyeParameters(eyeParams);

    } else {

        Model::updateRig(deltaTime, parentTransform);

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
        _rig->getJointRotation(geometry.headJointIndex, headOrientation);
        glm::vec3 eulers = safeEulerAngles(headOrientation);
        head->setBasePitch(glm::degrees(-eulers.x));
        head->setBaseYaw(glm::degrees(eulers.y));
        head->setBaseRoll(glm::degrees(-eulers.z));

        Rig::EyeParameters eyeParams;
        eyeParams.worldHeadOrientation = head->getFinalOrientationInWorldFrame();
        eyeParams.eyeLookAt = lookAt;
        eyeParams.eyeSaccade = glm::vec3(0.0f);
        eyeParams.modelRotation = getRotation();
        eyeParams.modelTranslation = getTranslation();
        eyeParams.leftEyeJointIndex = geometry.leftEyeJointIndex;
        eyeParams.rightEyeJointIndex = geometry.rightEyeJointIndex;

        _rig->updateFromEyeParameters(eyeParams);
     }
}

void SkeletonModel::updateAttitude() {
    setTranslation(_owningAvatar->getSkeletonPosition());
    setRotation(_owningAvatar->getOrientation() * Quaternions::Y_180);
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningAvatar->getScale());
}

// Called by Avatar::simulate after it has set the joint states (fullUpdate true if changed),
// but just before head has been simulated.
void SkeletonModel::simulate(float deltaTime, bool fullUpdate) {
    updateAttitude();
    setBlendshapeCoefficients(_owningAvatar->getHead()->getBlendshapeCoefficients());

    Model::simulate(deltaTime, fullUpdate);

    // let rig compute the model offset
    glm::vec3 registrationPoint;
    if (_rig->getModelRegistrationPoint(registrationPoint)) {
        setOffset(registrationPoint);
    }

    if (!isActive() || !_owningAvatar->isMyAvatar()) {
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
    int knuckleIndex = _rig->indexOfJoint("LeftHandMiddle1");
    int handIndex = _rig->indexOfJoint("LeftHand");
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
    int knuckleIndex = _rig->indexOfJoint("RightHandMiddle1");
    int handIndex = _rig->indexOfJoint("RightHand");
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

bool SkeletonModel::restoreLeftHandPosition(float fraction, float priority) {
    return restoreJointPosition(getLeftHandJointIndex(), fraction, priority);
}

bool SkeletonModel::getLeftShoulderPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getLastFreeJointIndex(getLeftHandJointIndex()), position);
}

float SkeletonModel::getLeftArmLength() const {
    return getLimbLength(getLeftHandJointIndex());
}

bool SkeletonModel::restoreRightHandPosition(float fraction, float priority) {
    return restoreJointPosition(getRightHandJointIndex(), fraction, priority);
}

bool SkeletonModel::getRightShoulderPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getLastFreeJointIndex(getRightHandJointIndex()), position);
}

float SkeletonModel::getRightArmLength() const {
    return getLimbLength(getRightHandJointIndex());
}

bool SkeletonModel::getHeadPosition(glm::vec3& headPosition) const {
    return isActive() && getJointPositionInWorldFrame(getFBXGeometry().headJointIndex, headPosition);
}

bool SkeletonModel::getNeckPosition(glm::vec3& neckPosition) const {
    return isActive() && getJointPositionInWorldFrame(getFBXGeometry().neckJointIndex, neckPosition);
}

bool SkeletonModel::getLocalNeckPosition(glm::vec3& neckPosition) const {
    return isActive() && getJointPosition(getFBXGeometry().neckJointIndex, neckPosition);
}

bool SkeletonModel::getEyeModelPositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = getFBXGeometry();

    if (getJointPosition(geometry.leftEyeJointIndex, firstEyePosition) &&
        getJointPosition(geometry.rightEyeJointIndex, secondEyePosition)) {
        return true;
    }
    // no eye joints; try to estimate based on head/neck joints
    glm::vec3 neckPosition, headPosition;
    if (getJointPosition(geometry.neckJointIndex, neckPosition) &&
        getJointPosition(geometry.headJointIndex, headPosition)) {
        const float EYE_PROPORTION = 0.6f;
        glm::vec3 baseEyePosition = glm::mix(neckPosition, headPosition, EYE_PROPORTION);
        glm::quat headRotation;
        getJointRotation(geometry.headJointIndex, headRotation);
        const float EYES_FORWARD = 0.25f;
        const float EYE_SEPARATION = 0.1f;
        float headHeight = glm::distance(neckPosition, headPosition);
        firstEyePosition = baseEyePosition + headRotation * glm::vec3(EYE_SEPARATION, 0.0f, EYES_FORWARD) * headHeight;
        secondEyePosition = baseEyePosition + headRotation * glm::vec3(-EYE_SEPARATION, 0.0f, EYES_FORWARD) * headHeight;
        return true;
    }
    return false;
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
    return _owningAvatar->getScale() * _defaultEyeModelPosition;
}

float DENSITY_OF_WATER = 1000.0f; // kg/m^3
float MIN_JOINT_MASS = 1.0f;
float VERY_BIG_MASS = 1.0e6f;

// virtual
void SkeletonModel::computeBoundingShape() {
    if (!isLoaded() || _rig->jointStatesEmpty()) {
        return;
    }

    const FBXGeometry& geometry = getFBXGeometry();
    if (geometry.joints.isEmpty() || geometry.rootJointIndex == -1) {
        // rootJointIndex == -1 if the avatar model has no skeleton
        return;
    }

    float radius, height;
    glm::vec3 offset;
    _rig->computeAvatarBoundingCapsule(geometry, radius, height, offset);
    float invScale = 1.0f / _owningAvatar->getUniformScale();
    _boundingCapsuleRadius = invScale * radius;
    _boundingCapsuleHeight = invScale * height;
    _boundingCapsuleLocalOffset = invScale * offset;
}

void SkeletonModel::renderBoundingCollisionShapes(gpu::Batch& batch, float scale, float alpha) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    // draw a blue sphere at the capsule top point
    glm::vec3 topPoint = _translation + getRotation() * (scale * (_boundingCapsuleLocalOffset + (0.5f * _boundingCapsuleHeight) * Vectors::UNIT_Y));

    batch.setModelTransform(Transform().setTranslation(topPoint).postScale(scale * _boundingCapsuleRadius));
    geometryCache->renderSolidSphereInstance(batch, glm::vec4(0.6f, 0.6f, 0.8f, alpha));

    // draw a yellow sphere at the capsule bottom point
    glm::vec3 bottomPoint = topPoint - glm::vec3(0.0f, scale * _boundingCapsuleHeight, 0.0f);
    glm::vec3 axis = topPoint - bottomPoint;

    batch.setModelTransform(Transform().setTranslation(bottomPoint).postScale(scale * _boundingCapsuleRadius));
    geometryCache->renderSolidSphereInstance(batch, glm::vec4(0.8f, 0.8f, 0.6f, alpha));

    // draw a green cylinder between the two points
    glm::vec3 origin(0.0f);
    batch.setModelTransform(Transform().setTranslation(bottomPoint));
    geometryCache->bindSimpleProgram(batch);
    Avatar::renderJointConnectingCone(batch, origin, axis, scale * _boundingCapsuleRadius, scale * _boundingCapsuleRadius,
                                      glm::vec4(0.6f, 0.8f, 0.6f, alpha));
}

bool SkeletonModel::hasSkeleton() {
    return isActive() ? getFBXGeometry().rootJointIndex != -1 : false;
}

void SkeletonModel::onInvalidate() {
}

