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

FaceModel::FaceModel(Head* owningHead) :
    _owningHead(owningHead)
{
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
    if (!owningAvatar->getSkeletonModel().getNeckParentRotation(neckParentRotation)) {
        neckParentRotation = owningAvatar->getOrientation();
    }
    setRotation(neckParentRotation);
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningHead->getScale() * MODEL_SCALE);
    
    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());
    
    if (isActive()) {
        setOffset(-_geometry->getFBXGeometry().neckPivot);
        Model::simulateInternal(deltaTime);
    }
}

void FaceModel::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(_rotation);
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState._transform * glm::translate(state._translation) *
        joint.preTransform * glm::mat4_cast(joint.preRotation)));
    state._rotation = glm::angleAxis(- RADIANS_PER_DEGREE * _owningHead->getFinalRoll(), glm::normalize(inverse * axes[2])) 
        * glm::angleAxis(RADIANS_PER_DEGREE * _owningHead->getFinalYaw(), glm::normalize(inverse * axes[1])) 
        * glm::angleAxis(- RADIANS_PER_DEGREE * _owningHead->getFinalPitch(), glm::normalize(inverse * axes[0])) 
        * joint.rotation;
}

void FaceModel::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // likewise with the eye joints
    glm::mat4 inverse = glm::inverse(parentState._transform * glm::translate(state._translation) *
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation));
    glm::vec3 front = glm::vec3(inverse * glm::vec4(_owningHead->getFinalOrientation() * IDENTITY_FRONT, 0.0f));
    glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(_owningHead->getLookAtPosition() +
        _owningHead->getSaccade() - _translation, 1.0f));
    glm::quat between = rotationBetween(front, lookAt);
    const float MAX_ANGLE = 30.0f * RADIANS_PER_DEGREE;
    state._rotation = glm::angleAxis(glm::clamp(glm::angle(between), -MAX_ANGLE, MAX_ANGLE), glm::axis(between)) *
        joint.rotation;
}

void FaceModel::updateJointState(int index) {
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();
    if (joint.parentIndex != -1) {
        const JointState& parentState = _jointStates.at(joint.parentIndex);
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        if (index == geometry.neckJointIndex) {
            maybeUpdateNeckRotation(parentState, joint, state);    
                
        } else if (index == geometry.leftEyeJointIndex || index == geometry.rightEyeJointIndex) {
            maybeUpdateEyeRotation(parentState, joint, state);
        }
    }

    Model::updateJointState(index);
}

bool FaceModel::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    return getJointPosition(geometry.leftEyeJointIndex, firstEyePosition) &&
        getJointPosition(geometry.rightEyeJointIndex, secondEyePosition);
}
