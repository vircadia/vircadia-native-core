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

FaceModel::FaceModel(Head* owningHead) :
    _owningHead(owningHead)
{
}

void FaceModel::simulate(float deltaTime) {
    if (!isActive()) {
        return;
    }
    Avatar* owningAvatar = static_cast<Avatar*>(_owningHead->_owningAvatar);
    glm::vec3 neckPosition;
    glm::vec3 modelTranslation;
    if (!owningAvatar->getSkeletonModel().getNeckPosition(neckPosition)) {
        neckPosition = owningAvatar->getSkeleton().joint[AVATAR_JOINT_NECK_BASE].position;
        const glm::vec3 OLD_SKELETON_MODEL_TRANSLATION(0.0f, -60.0f, 40.0f);
        modelTranslation = OLD_SKELETON_MODEL_TRANSLATION;
    }
    setTranslation(neckPosition);
    glm::quat neckRotation;
    if (!owningAvatar->getSkeletonModel().getNeckRotation(neckRotation)) {
        neckRotation = owningAvatar->getSkeleton().joint[AVATAR_JOINT_NECK_BASE].absoluteRotation *
            glm::angleAxis(180.0f, 0.0f, 1.0f, 0.0f);
    }
    setRotation(neckRotation);
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningHead->getScale() * MODEL_SCALE);
    setOffset(modelTranslation - _geometry->getFBXGeometry().neckPivot);
    
    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());
    
    Model::simulate(deltaTime);
}

void FaceModel::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(_rotation);
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.transform *
        joint.preTransform * glm::mat4_cast(joint.preRotation)));
    state.rotation = glm::angleAxis(-_owningHead->getRoll(), glm::normalize(inverse * axes[2])) *
        glm::angleAxis(_owningHead->getYaw(), glm::normalize(inverse * axes[1])) *
        glm::angleAxis(-_owningHead->getPitch(), glm::normalize(inverse * axes[0])) * joint.rotation;
}

void FaceModel::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // likewise with the eye joints
    glm::mat4 inverse = glm::inverse(parentState.transform *
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation));
    glm::vec3 front = glm::vec3(inverse * glm::vec4(_owningHead->getOrientation() * IDENTITY_FRONT, 0.0f));
    glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(_owningHead->getLookAtPosition() +
        _owningHead->getSaccade() - _translation, 1.0f));
    glm::quat between = rotationBetween(front, lookAt);
    const float MAX_ANGLE = 30.0f;
    state.rotation = glm::angleAxis(glm::clamp(glm::angle(between), -MAX_ANGLE, MAX_ANGLE), glm::axis(between)) *
        joint.rotation;
}
