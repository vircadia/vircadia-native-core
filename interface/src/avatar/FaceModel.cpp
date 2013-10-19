//
//  FaceModel.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

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

    const Skeleton& skeleton = static_cast<Avatar*>(_owningHead->_owningAvatar)->getSkeleton();
    setTranslation(skeleton.joint[AVATAR_JOINT_NECK_BASE].position);
    setRotation(skeleton.joint[AVATAR_JOINT_NECK_BASE].absoluteRotation);
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(-1.0f, 1.0f, -1.0f) * _owningHead->getScale() * MODEL_SCALE);
    const glm::vec3 MODEL_TRANSLATION(0.0f, -60.0f, 40.0f); // temporary fudge factor
    setOffset(MODEL_TRANSLATION - _geometry->getFBXGeometry().neckPivot);
    
    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());
    
    Model::simulate(deltaTime);
}

void FaceModel::maybeUpdateNeckRotation(const FBXJoint& joint, JointState& state) {
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(getRotation());
    glm::mat3 inverse = glm::inverse(glm::mat3(_jointStates[joint.parentIndex].transform *
        joint.preRotation * glm::mat4_cast(joint.rotation)));
    state.rotation = glm::angleAxis(_owningHead->getRoll(), glm::normalize(inverse * axes[2])) *
        glm::angleAxis(_owningHead->getYaw(), glm::normalize(inverse * axes[1])) *
        glm::angleAxis(_owningHead->getPitch(), glm::normalize(inverse * axes[0])) * joint.rotation;
}

void FaceModel::maybeUpdateEyeRotation(const FBXJoint& joint, JointState& state) {
    // get the lookat position in joint space and use it to adjust the rotation
    glm::mat4 inverse = glm::inverse(_jointStates[joint.parentIndex].transform *
        joint.preRotation * glm::mat4_cast(joint.rotation));
    glm::vec3 front = glm::vec3(inverse * glm::vec4(_owningHead->getOrientation() * IDENTITY_FRONT, 0.0f));
    glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(_owningHead->getLookAtPosition() +
        _owningHead->getSaccade(), 1.0f));
    state.rotation = rotationBetween(front, lookAt) * joint.rotation;
}
