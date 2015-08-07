//
//  FaceModel.cpp
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/transform.hpp>

#include "Avatar.h"
#include "FaceModel.h"
#include "Head.h"
#include "Menu.h"

FaceModel::FaceModel(Head* owningHead, RigPointer rig) :
    Model(rig, nullptr),
    _owningHead(owningHead)
{
    assert(_rig);
}

void FaceModel::simulate(float deltaTime, bool fullUpdate) {
    updateGeometry();

    Avatar* owningAvatar = static_cast<Avatar*>(_owningHead->_owningAvatar);
    glm::vec3 neckPosition;
    if (!owningAvatar->getSkeletonModel().getNeckPosition(neckPosition)) {
        neckPosition = owningAvatar->getPosition();
    }
    setTranslation(neckPosition);
    glm::quat neckParentRotation;
    if (!owningAvatar->getSkeletonModel().getNeckParentRotationFromDefaultOrientation(neckParentRotation)) {
        neckParentRotation = owningAvatar->getOrientation();
    }
    setRotation(neckParentRotation);
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningHead->getScale());

    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());

    // FIXME - this is very expensive, we shouldn't do it if we don't have to
    //invalidCalculatedMeshBoxes();

    if (isActive()) {
        setOffset(-_geometry->getFBXGeometry().neckPivot);

        for (int i = 0; i < _rig->getJointStateCount(); i++) {
            maybeUpdateNeckAndEyeRotation(i);
        }

        Model::simulateInternal(deltaTime);
    }
}

void FaceModel::maybeUpdateNeckRotation(const JointState& parentState, const JointState& state, int index) {
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(glm::quat());
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.getTransform() *
                                               glm::translate(_rig->getJointDefaultTranslationInConstrainedFrame(index)) *
                                               state.getPreTransform() * glm::mat4_cast(state.getPreRotation())));
    glm::vec3 pitchYawRoll = safeEulerAngles(_owningHead->getFinalOrientationInLocalFrame());
    glm::vec3 lean = glm::radians(glm::vec3(_owningHead->getFinalLeanForward(),
                                            _owningHead->getTorsoTwist(),
                                            _owningHead->getFinalLeanSideways()));
    pitchYawRoll -= lean;
    _rig->setJointRotationInConstrainedFrame(index,
                                             glm::angleAxis(-pitchYawRoll.z, glm::normalize(inverse * axes[2]))
                                             * glm::angleAxis(pitchYawRoll.y, glm::normalize(inverse * axes[1]))
                                             * glm::angleAxis(-pitchYawRoll.x, glm::normalize(inverse * axes[0]))
                                             * state.getOriginalRotation(), DEFAULT_PRIORITY);
}

void FaceModel::maybeUpdateEyeRotation(Model* model, const JointState& parentState, const JointState& state, int index) {
    // likewise with the eye joints
    // NOTE: at the moment we do the math in the world-frame, hence the inverse transform is more complex than usual.
    glm::mat4 inverse = glm::inverse(glm::mat4_cast(model->getRotation()) * parentState.getTransform() *
                                     glm::translate(_rig->getJointDefaultTranslationInConstrainedFrame(index)) *
                                     state.getPreTransform() * glm::mat4_cast(state.getPreRotation() * state.getOriginalRotation()));
    glm::vec3 front = glm::vec3(inverse * glm::vec4(_owningHead->getFinalOrientationInWorldFrame() * IDENTITY_FRONT, 0.0f));
    glm::vec3 lookAtDelta = _owningHead->getCorrectedLookAtPosition() - model->getTranslation();
    glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(lookAtDelta + glm::length(lookAtDelta) * _owningHead->getSaccade(), 1.0f));
    glm::quat between = rotationBetween(front, lookAt);
    const float MAX_ANGLE = 30.0f * RADIANS_PER_DEGREE;
    _rig->setJointRotationInConstrainedFrame(index, glm::angleAxis(glm::clamp(glm::angle(between),
                                                                              -MAX_ANGLE, MAX_ANGLE), glm::axis(between)) *
                                             state.getOriginalRotation(), DEFAULT_PRIORITY);
}

void FaceModel::maybeUpdateNeckAndEyeRotation(int index) {
    const JointState& state = _rig->getJointState(index);
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const int parentIndex = state.getParentIndex();

    // guard against out-of-bounds access to _jointStates
    if (parentIndex != -1 && parentIndex >= 0 && parentIndex < _rig->getJointStateCount()) {
        const JointState& parentState = _rig->getJointState(parentIndex);
        if (index == geometry.neckJointIndex) {
            maybeUpdateNeckRotation(parentState, state, index);

        } else if (index == geometry.leftEyeJointIndex || index == geometry.rightEyeJointIndex) {
            maybeUpdateEyeRotation(this, parentState, state, index);
        }
    }
}

bool FaceModel::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    return getJointPositionInWorldFrame(geometry.leftEyeJointIndex, firstEyePosition) &&
        getJointPositionInWorldFrame(geometry.rightEyeJointIndex, secondEyePosition);
}
