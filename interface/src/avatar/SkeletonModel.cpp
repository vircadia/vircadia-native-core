//
//  SkeletonModel.cpp
//  interface
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/transform.hpp>

#include "Avatar.h"
#include "SkeletonModel.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar) :
    _owningAvatar(owningAvatar)
{
}

void SkeletonModel::simulate(float deltaTime) {
    if (!isActive()) {
        return;
    }
    
    setTranslation(_owningAvatar->getPosition());
    setRotation(_owningAvatar->getOrientation() * glm::angleAxis(180.0f, 0.0f, 1.0f, 0.0f));
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningAvatar->getScale() * MODEL_SCALE);
    
    Model::simulate(deltaTime);
}

bool SkeletonModel::render(float alpha) {
    if (_jointStates.isEmpty()) {
        return false;
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    
    for (int i = 0; i < _jointStates.size(); i++) {
        const JointState& joint = _jointStates.at(i);
        
        glPushMatrix();
        glMultMatrixf((const GLfloat*)&joint.transform);
        
        if (i == geometry.rootJointIndex) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        glutSolidSphere(0.05f, 10, 10);
        
        glPopMatrix();    
    }
    
    Model::render(alpha);
    
    return true;
}

void SkeletonModel::updateJointState(int index) {
    Model::updateJointState(index);
    
    if (index == _geometry->getFBXGeometry().rootJointIndex) {
        JointState& state = _jointStates[index];
        state.transform[3][0] = _translation.x;
        state.transform[3][1] = _translation.y;
        state.transform[3][2] = _translation.z;
    }
}

void SkeletonModel::maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(_rotation);
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.transform *
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation)));
    state.rotation = glm::angleAxis(-_owningAvatar->getHead().getLeanSideways(), glm::normalize(inverse * axes[2])) *
        glm::angleAxis(-_owningAvatar->getHead().getLeanForward(), glm::normalize(inverse * axes[0])) * joint.rotation;
}

