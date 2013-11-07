//
//  SkeletonModel.cpp
//  interface
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/transform.hpp>

#include "Application.h"
#include "Avatar.h"
#include "SkeletonModel.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar) :
    _owningAvatar(owningAvatar) {
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
    
    const float HAND_RESTORATION_RATE = 0.25f;
    switch (_owningAvatar->getHand().getNumPalms()) {
        case 0: // no Leap data; set hands from mouse
            if (_owningAvatar->getHandState() == HAND_STATE_NULL) {
                restoreRightHandPosition(HAND_RESTORATION_RATE);
            } else {
                setRightHandPosition(_owningAvatar->getHandPosition());
            }
            restoreLeftHandPosition(HAND_RESTORATION_RATE);
            break;
                
        case 1: // right hand only
            applyPalmData(_geometry->getFBXGeometry().rightHandJointIndex, _owningAvatar->getHand().getPalms()[0]);
            restoreLeftHandPosition(HAND_RESTORATION_RATE);
            break;    
            
        default: // both hands
            applyPalmData(_geometry->getFBXGeometry().leftHandJointIndex, _owningAvatar->getHand().getPalms()[0]);
            applyPalmData(_geometry->getFBXGeometry().rightHandJointIndex, _owningAvatar->getHand().getPalms()[1]);
            break;
    }
}

bool SkeletonModel::render(float alpha) {
    if (_jointStates.isEmpty()) {
        return false;
    }
    
    // only render the balls and sticks if the skeleton has no meshes
    if (_meshStates.isEmpty()) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        
        glm::vec3 skinColor, darkSkinColor;
        _owningAvatar->getSkinColors(skinColor, darkSkinColor);
        
        for (int i = 0; i < _jointStates.size(); i++) {
            glPushMatrix();
            
            glm::vec3 position;
            getJointPosition(i, position);
            Application::getInstance()->loadTranslatedViewMatrix(position);
            
            glm::quat rotation;
            getJointRotation(i, rotation);
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::angle(rotation), axis.x, axis.y, axis.z);
            
            glColor4f(skinColor.r, skinColor.g, skinColor.b, alpha);
            const float BALL_RADIUS = 0.005f;
            const int BALL_SUBDIVISIONS = 10;
            glutSolidSphere(BALL_RADIUS * _owningAvatar->getScale(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);
            
            glPopMatrix();
            
            int parentIndex = geometry.joints[i].parentIndex;
            if (parentIndex == -1) {
                continue;
            }
            glColor4f(darkSkinColor.r, darkSkinColor.g, darkSkinColor.b, alpha);
            
            glm::vec3 parentPosition;
            getJointPosition(parentIndex, parentPosition);
            const float STICK_RADIUS = BALL_RADIUS * 0.1f;
            Avatar::renderJointConnectingCone(parentPosition, position, STICK_RADIUS * _owningAvatar->getScale(),
                                              STICK_RADIUS * _owningAvatar->getScale());
        }
    }
    
    Model::render(alpha);
    
    return true;
}

void SkeletonModel::applyPalmData(int jointIndex, const PalmData& palm) {
    setJointPosition(jointIndex, palm.getPosition());
}

void SkeletonModel::updateJointState(int index) {
    Model::updateJointState(index);
    
    if (index == _geometry->getFBXGeometry().rootJointIndex) {
        JointState& state = _jointStates[index];
        state.transform[3][0] = 0.0f;
        state.transform[3][1] = 0.0f;
        state.transform[3][2] = 0.0f;
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

