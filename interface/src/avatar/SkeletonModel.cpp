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

    // find the left and rightmost active Leap palms
    HandData& hand = _owningAvatar->getHand();
    int leftPalmIndex = -1;
    float leftPalmX = FLT_MAX;
    int rightPalmIndex = -1;    
    float rightPalmX = -FLT_MAX;
    for (int i = 0; i < hand.getNumPalms(); i++) {
        if (hand.getPalms()[i].isActive()) {
            float x = hand.getPalms()[i].getRawPosition().x;
            if (x < leftPalmX) {
                leftPalmIndex = i;
                leftPalmX = x;
            }
            if (x > rightPalmX) {
                rightPalmIndex = i;
                rightPalmX = x;
            }
        }
    }
    
    const float HAND_RESTORATION_RATE = 0.25f;
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (leftPalmIndex == -1) {
        // no Leap data; set hands from mouse
        if (_owningAvatar->getHandState() == HAND_STATE_NULL) {
            restoreRightHandPosition(HAND_RESTORATION_RATE);
        } else {
            setRightHandPosition(_owningAvatar->getHandPosition());
        }
        restoreLeftHandPosition(HAND_RESTORATION_RATE);
    
    } else if (leftPalmIndex == rightPalmIndex) {
        // right hand only
        applyPalmData(geometry.rightHandJointIndex, geometry.rightFingertipJointIndices, hand.getPalms()[leftPalmIndex]);
        restoreLeftHandPosition(HAND_RESTORATION_RATE);
        
    } else {
        applyPalmData(geometry.leftHandJointIndex, geometry.leftFingertipJointIndices, hand.getPalms()[leftPalmIndex]);
        applyPalmData(geometry.rightHandJointIndex, geometry.rightFingertipJointIndices, hand.getPalms()[rightPalmIndex]);
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

class IndexValue {
public:
    int index;
    float value;
};

bool operator<(const IndexValue& firstIndex, const IndexValue& secondIndex) {
    return firstIndex.value < secondIndex.value;
}

void SkeletonModel::applyPalmData(int jointIndex, const QVector<int>& fingertipJointIndices, PalmData& palm) {
    setJointPosition(jointIndex, palm.getPosition());
    setJointRotation(jointIndex, rotationBetween(_rotation * IDENTITY_UP, -palm.getNormal()) * _rotation *
        glm::angleAxis(90.0f, 0.0f, jointIndex == _geometry->getFBXGeometry().rightHandJointIndex ? 1.0f : -1.0f, 0.0f), true);
    
    // no point in continuing if there are no fingers
    if (true || palm.getNumFingers() == 0) {
        return;
    }
     
    // sort the finger indices by raw x
    QVector<IndexValue> fingerIndices;
    for (int i = 0; i < palm.getNumFingers(); i++) {
        IndexValue indexValue = { i, palm.getFingers()[i].getTipRawPosition().x };
        fingerIndices.append(indexValue);
    }
    //qSort(fingerIndices.begin(), fingerIndices.end());
    
    // likewise with the joint indices and relative x
    QVector<IndexValue> jointIndices;
    foreach (int index, fingertipJointIndices) {
        glm::vec3 position = glm::inverse(_rotation) * extractTranslation(_jointStates[index].transform);
        IndexValue indexValue = { index, position.x };
        jointIndices.append(indexValue);
    }
    //qSort(jointIndices.begin(), jointIndices.end());
    
    // match them up as best we can
    float proportion = fingerIndices.size() / (float)jointIndices.size();
    for (int i = 0; i < jointIndices.size(); i++) {
        int fingerIndex = fingerIndices.at(roundf(i * proportion)).index;
        setJointPosition(jointIndices.at(i).index, palm.getFingers()[fingerIndex].getTipPosition(), jointIndex);
    }
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

