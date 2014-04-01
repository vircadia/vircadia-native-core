//
//  FaceModel.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
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

void FaceModel::simulate(float deltaTime) {
    bool geometryIsUpToDate = updateGeometry();
    if (!geometryIsUpToDate) {
        return;
    }
    Avatar* owningAvatar = static_cast<Avatar*>(_owningHead->_owningAvatar);
    glm::vec3 neckPosition;
    if (!owningAvatar->getSkeletonModel().getNeckPosition(neckPosition)) {
        neckPosition = owningAvatar->getPosition();
    }
    setTranslation(neckPosition);
    glm::quat neckRotation;
    if (!owningAvatar->getSkeletonModel().getNeckRotation(neckRotation)) {
        neckRotation = owningAvatar->getOrientation();
    }
    setRotation(neckRotation);
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningHead->getScale() * MODEL_SCALE);
    
    setOffset(-_geometry->getFBXGeometry().neckPivot);
    
    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());
    
    Model::simulateInternal(deltaTime);
}

void FaceModel::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(_rotation);
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.transform * glm::translate(state.translation) *
        joint.preTransform * glm::mat4_cast(joint.preRotation)));
    state.rotation = glm::angleAxis(- RADIANS_PER_DEGREE * _owningHead->getFinalRoll(), glm::normalize(inverse * axes[2])) 
        * glm::angleAxis(RADIANS_PER_DEGREE * _owningHead->getFinalYaw(), glm::normalize(inverse * axes[1])) 
        * glm::angleAxis(- RADIANS_PER_DEGREE * _owningHead->getFinalPitch(), glm::normalize(inverse * axes[0])) 
        * joint.rotation;
}

void FaceModel::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // likewise with the eye joints
    glm::mat4 inverse = glm::inverse(parentState.transform * glm::translate(state.translation) *
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation));
    glm::vec3 front = glm::vec3(inverse * glm::vec4(_owningHead->getFinalOrientation() * IDENTITY_FRONT, 0.0f));
    glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(_owningHead->getLookAtPosition() +
        _owningHead->getSaccade() - _translation, 1.0f));
    glm::quat between = rotationBetween(front, lookAt);
    const float MAX_ANGLE = 30.0f * RADIANS_PER_DEGREE;
    state.rotation = glm::angleAxis(glm::clamp(glm::angle(between), -MAX_ANGLE, MAX_ANGLE), glm::axis(between)) *
        joint.rotation;
}
