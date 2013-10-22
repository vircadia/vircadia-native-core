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
    if (!owningAvatar->getSkeletonModel().getNeckPosition(neckPosition)) {
        neckPosition = owningAvatar->getSkeleton().joint[AVATAR_JOINT_NECK_BASE].position;
    }
    setTranslation(neckPosition);
    glm::quat neckRotation;
    if (!owningAvatar->getSkeletonModel().getNeckRotation(neckRotation)) {
        neckRotation = owningAvatar->getSkeleton().joint[AVATAR_JOINT_NECK_BASE].absoluteRotation;
    }
    setRotation(neckRotation);
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningHead->getScale() * MODEL_SCALE);
    const glm::vec3 MODEL_TRANSLATION(0.0f, -60.0f, 40.0f); // temporary fudge factor
    setOffset(MODEL_TRANSLATION - _geometry->getFBXGeometry().neckPivot);
    
    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());
    
    Model::simulate(deltaTime);
}

void FaceModel::updateJointState(int index) {
    JointState& state = _jointStates[index];
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const FBXJoint& joint = geometry.joints.at(index);
    
    glm::quat combinedRotation = joint.preRotation * state.rotation * joint.postRotation;
    if (joint.parentIndex == -1) {
        glm::mat4 baseTransform = glm::translate(_translation) * glm::mat4_cast(_rotation) *
            glm::scale(_scale) * glm::translate(_offset);
        
        state.transform = baseTransform * geometry.offset * joint.preTransform *
            glm::mat4_cast(combinedRotation) * joint.postTransform;
        state.combinedRotation = _rotation * combinedRotation;
    
    } else {
        if (index == geometry.neckJointIndex) {
            // get the rotation axes in joint space and use them to adjust the rotation
            glm::mat3 axes = glm::mat3_cast(getRotation());
            glm::mat3 inverse = glm::inverse(glm::mat3(_jointStates[joint.parentIndex].transform *
                joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation)));
            state.rotation = glm::angleAxis(_owningHead->getRoll(), glm::normalize(inverse * axes[2])) *
                glm::angleAxis(_owningHead->getYaw(), glm::normalize(inverse * axes[1])) *
                glm::angleAxis(_owningHead->getPitch(), glm::normalize(inverse * axes[0])) * joint.rotation;
            
        } else if (index == geometry.leftEyeJointIndex || index == geometry.rightEyeJointIndex) {
            // likewise with the eye joints
            glm::mat4 inverse = glm::inverse(_jointStates[joint.parentIndex].transform *
                joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation));
            glm::vec3 front = glm::vec3(inverse * glm::vec4(_owningHead->getOrientation() * IDENTITY_FRONT, 0.0f));
            glm::vec3 lookAt = glm::vec3(inverse * glm::vec4(_owningHead->getLookAtPosition() +
                _owningHead->getSaccade(), 1.0f));
            state.rotation = rotationBetween(front, lookAt) * joint.rotation;
        }     
        const JointState& parentState = _jointStates.at(joint.parentIndex);
        state.transform = parentState.transform * joint.preTransform *
            glm::mat4_cast(combinedRotation) * joint.postTransform;
        state.combinedRotation = parentState.combinedRotation * combinedRotation;
    }
}
