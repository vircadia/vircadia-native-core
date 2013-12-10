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
#include "Menu.h"
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
    int leftPalmIndex, rightPalmIndex;   
    HandData& hand = _owningAvatar->getHand(); 
    hand.getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);
    
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
        applyPalmData(geometry.rightHandJointIndex, geometry.rightFingerJointIndices, geometry.rightFingertipJointIndices, 
            hand.getPalms()[leftPalmIndex]);
        restoreLeftHandPosition(HAND_RESTORATION_RATE);
        
    } else {
        applyPalmData(geometry.leftHandJointIndex, geometry.leftFingerJointIndices, geometry.leftFingertipJointIndices,
            hand.getPalms()[leftPalmIndex]);
        applyPalmData(geometry.rightHandJointIndex, geometry.rightFingerJointIndices, geometry.rightFingertipJointIndices,
            hand.getPalms()[rightPalmIndex]);
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
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::CollisionProxies)) {
        renderCollisionProxies(alpha);
    }
    
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

void SkeletonModel::applyPalmData(int jointIndex, const QVector<int>& fingerJointIndices,
        const QVector<int>& fingertipJointIndices, PalmData& palm) {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    setJointPosition(jointIndex, palm.getPosition());
    float sign = (jointIndex == geometry.rightHandJointIndex) ? 1.0f : -1.0f;
    glm::quat palmRotation;
    getJointRotation(jointIndex, palmRotation, true);
    applyRotationDelta(jointIndex, rotationBetween(palmRotation * geometry.palmDirection, palm.getNormal()), false);
    getJointRotation(jointIndex, palmRotation, true);
    
    // sort the finger indices by raw x, get the average direction
    QVector<IndexValue> fingerIndices;
    glm::vec3 direction;
    for (int i = 0; i < palm.getNumFingers(); i++) {
        glm::vec3 fingerVector = palm.getFingers()[i].getTipPosition() - palm.getPosition();
        float length = glm::length(fingerVector);
        if (length > EPSILON) {
            direction += fingerVector / length;
        }
        fingerVector = glm::inverse(palmRotation) * fingerVector * -sign;
        IndexValue indexValue = { i, atan2f(fingerVector.z, fingerVector.x) };
        fingerIndices.append(indexValue);
    }
    qSort(fingerIndices.begin(), fingerIndices.end());
    
    // rotate palm according to average finger direction
    float directionLength = glm::length(direction);
    const int MIN_ROTATION_FINGERS = 3;
    if (directionLength > EPSILON && palm.getNumFingers() >= MIN_ROTATION_FINGERS) {
        applyRotationDelta(jointIndex, rotationBetween(palmRotation * glm::vec3(-sign, 0.0f, 0.0f), direction), false);
        getJointRotation(jointIndex, palmRotation, true);
    }
    
    // no point in continuing if there are no fingers
    if (palm.getNumFingers() == 0 || fingerJointIndices.isEmpty()) {
        return;
    }
     
    // match them up as best we can
    float proportion = fingerIndices.size() / (float)fingerJointIndices.size();
    for (int i = 0; i < fingerJointIndices.size(); i++) {
        int fingerIndex = fingerIndices.at(roundf(i * proportion)).index;
        glm::vec3 fingerVector = palm.getFingers()[fingerIndex].getTipPosition() -
            palm.getFingers()[fingerIndex].getRootPosition();
        
        int fingerJointIndex = fingerJointIndices.at(i);
        int fingertipJointIndex = fingertipJointIndices.at(i);
        glm::vec3 jointVector = extractTranslation(geometry.joints.at(fingertipJointIndex).bindTransform) -
            extractTranslation(geometry.joints.at(fingerJointIndex).bindTransform);
        
        setJointRotation(fingerJointIndex, rotationBetween(palmRotation * jointVector, fingerVector) * palmRotation, true);
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

