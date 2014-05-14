//
//  SkeletonModel.cpp
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/transform.hpp>

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Menu.h"
#include "SkeletonModel.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar) : 
    _owningAvatar(owningAvatar) {
}

void SkeletonModel::simulate(float deltaTime, bool fullUpdate) {
    setTranslation(_owningAvatar->getPosition());
    setRotation(_owningAvatar->getOrientation() * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f)));
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningAvatar->getScale() * MODEL_SCALE);
    
    Model::simulate(deltaTime, fullUpdate);
    
    if (!(isActive() && _owningAvatar->isMyAvatar())) {
        return; // only simulate for own avatar
    }

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    PrioVR* prioVR = Application::getInstance()->getPrioVR();
    if (prioVR->isActive()) {
        for (int i = 0; i < prioVR->getJointRotations().size(); i++) {
            int humanIKJointIndex = prioVR->getHumanIKJointIndices().at(i);
            if (humanIKJointIndex == -1) {
                continue;
            }
            int jointIndex = geometry.humanIKJointIndices.at(humanIKJointIndex);
            if (jointIndex != -1) {
                setJointRotation(jointIndex, _rotation * prioVR->getJointRotations().at(i), true);
            }
        }
        return;
    }

    // find the left and rightmost active palms
    int leftPalmIndex, rightPalmIndex;
    Hand* hand = _owningAvatar->getHand();
    hand->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);

    const float HAND_RESTORATION_RATE = 0.25f;    
    if (leftPalmIndex == -1) {
        // palms are not yet set, use mouse
        if (_owningAvatar->getHandState() == HAND_STATE_NULL) {
            restoreRightHandPosition(HAND_RESTORATION_RATE);
        } else {
            applyHandPosition(geometry.rightHandJointIndex, _owningAvatar->getHandPosition());
        }
        restoreLeftHandPosition(HAND_RESTORATION_RATE);

    } else if (leftPalmIndex == rightPalmIndex) {
        // right hand only
        applyPalmData(geometry.rightHandJointIndex, hand->getPalms()[leftPalmIndex]);
        restoreLeftHandPosition(HAND_RESTORATION_RATE);

    } else {
        applyPalmData(geometry.leftHandJointIndex, hand->getPalms()[leftPalmIndex]);
        applyPalmData(geometry.rightHandJointIndex, hand->getPalms()[rightPalmIndex]);
    }
}

void SkeletonModel::getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const {
    if (jointIndex < 0 || jointIndex >= int(_jointShapes.size())) {
        return;
    }
    if (jointIndex == getLeftHandJointIndex()
        || jointIndex == getRightHandJointIndex()) {
        // get all shapes that have this hand as an ancestor in the skeleton heirarchy
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        for (int i = 0; i < _jointStates.size(); i++) {
            const FBXJoint& joint = geometry.joints[i];
            int parentIndex = joint.parentIndex;
            if (i == jointIndex) {
                // this shape is the hand
                shapes.push_back(_jointShapes[i]);
                if (parentIndex != -1) {
                    // also add the forearm
                    shapes.push_back(_jointShapes[parentIndex]);
                }
            } else {
                while (parentIndex != -1) {
                    if (parentIndex == jointIndex) {
                        // this shape is a child of the hand
                        shapes.push_back(_jointShapes[i]);
                        break;
                    }
                    parentIndex = geometry.joints[parentIndex].parentIndex;
                }
            }
        }
    }
}

void SkeletonModel::getBodyShapes(QVector<const Shape*>& shapes) const {
    // for now we push a single bounding shape, 
    // but later we could push a subset of joint shapes
    shapes.push_back(&_boundingShape);
}

void SkeletonModel::renderIKConstraints() {
    renderJointConstraints(getRightHandJointIndex());
    renderJointConstraints(getLeftHandJointIndex());
}

class IndexValue {
public:
    int index;
    float value;
};

bool operator<(const IndexValue& firstIndex, const IndexValue& secondIndex) {
    return firstIndex.value < secondIndex.value;
}

void SkeletonModel::applyHandPosition(int jointIndex, const glm::vec3& position) {
    if (jointIndex == -1) {
        return;
    }
    setJointPosition(jointIndex, position);

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    glm::vec3 handPosition, elbowPosition;
    getJointPosition(jointIndex, handPosition);
    getJointPosition(geometry.joints.at(jointIndex).parentIndex, elbowPosition);
    glm::vec3 forearmVector = handPosition - elbowPosition;
    float forearmLength = glm::length(forearmVector);
    if (forearmLength < EPSILON) {
        return;
    }
    glm::quat handRotation;
    getJointRotation(jointIndex, handRotation, true);

    // align hand with forearm
    float sign = (jointIndex == geometry.rightHandJointIndex) ? 1.0f : -1.0f;
    applyRotationDelta(jointIndex, rotationBetween(handRotation * glm::vec3(-sign, 0.0f, 0.0f), forearmVector));
}

void SkeletonModel::applyPalmData(int jointIndex, PalmData& palm) {
    if (jointIndex == -1) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    float sign = (jointIndex == geometry.rightHandJointIndex) ? 1.0f : -1.0f;
    int parentJointIndex = geometry.joints.at(jointIndex).parentIndex;
    if (parentJointIndex == -1) {
        return;
    }
    
    // rotate palm to align with its normal (normal points out of hand's palm)
    glm::quat palmRotation;
    if (Menu::getInstance()->isOptionChecked(MenuOption::AlignForearmsWithWrists)) {
        getJointRotation(parentJointIndex, palmRotation, true);
    } else {
        getJointRotation(jointIndex, palmRotation, true);
    }
    palmRotation = rotationBetween(palmRotation * geometry.palmDirection, palm.getNormal()) * palmRotation;
    
    // rotate palm to align with finger direction
    glm::vec3 direction = palm.getFingerDirection();
    palmRotation = rotationBetween(palmRotation * glm::vec3(-sign, 0.0f, 0.0f), direction) * palmRotation;

    // set hand position, rotation
    if (Menu::getInstance()->isOptionChecked(MenuOption::AlignForearmsWithWrists)) {
        glm::vec3 forearmVector = palmRotation * glm::vec3(sign, 0.0f, 0.0f);
        setJointPosition(parentJointIndex, palm.getPosition() + forearmVector *
            geometry.joints.at(jointIndex).distanceToParent * extractUniformScale(_scale));
        setJointRotation(parentJointIndex, palmRotation, true);
        _jointStates[jointIndex].rotation = glm::quat();
        
    } else {
        setJointPosition(jointIndex, palm.getPosition(), palmRotation, true);
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
    if (!_owningAvatar->isMyAvatar() || Application::getInstance()->getPrioVR()->isActive()) {
        return;
    }
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(_rotation);
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.transform * glm::translate(state.translation) * 
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation)));
    state.rotation = glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getFinalLeanSideways(), 
        glm::normalize(inverse * axes[2])) * glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getFinalLeanForward(), 
        glm::normalize(inverse * axes[0])) * joint.rotation;
}

void SkeletonModel::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    _owningAvatar->getHead()->getFaceModel().maybeUpdateNeckRotation(parentState, joint, state);
}

void SkeletonModel::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    _owningAvatar->getHead()->getFaceModel().maybeUpdateEyeRotation(parentState, joint, state);
}

void SkeletonModel::renderJointConstraints(int jointIndex) {
    if (jointIndex == -1) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const float BASE_DIRECTION_SIZE = 300.0f;
    float directionSize = BASE_DIRECTION_SIZE * extractUniformScale(_scale);
    glLineWidth(3.0f);
    do {
        const FBXJoint& joint = geometry.joints.at(jointIndex);
        const JointState& jointState = _jointStates.at(jointIndex);
        glm::vec3 position = extractTranslation(jointState.transform) + _translation;
        
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::quat parentRotation = (joint.parentIndex == -1) ? _rotation : _jointStates.at(joint.parentIndex).combinedRotation;
        glm::vec3 rotationAxis = glm::axis(parentRotation);
        glRotatef(glm::degrees(glm::angle(parentRotation)), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        float fanScale = directionSize * 0.75f;
        glScalef(fanScale, fanScale, fanScale);
        const int AXIS_COUNT = 3;
        for (int i = 0; i < AXIS_COUNT; i++) {
            if (joint.rotationMin[i] <= -PI + EPSILON && joint.rotationMax[i] >= PI - EPSILON) {
                continue; // unconstrained
            }
            glm::vec3 axis;
            axis[i] = 1.0f;
            
            glm::vec3 otherAxis;
            if (i == 0) {
                otherAxis.y = 1.0f;
            } else {
                otherAxis.x = 1.0f;
            }
            glColor4f(otherAxis.r, otherAxis.g, otherAxis.b, 0.75f);
        
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0f, 0.0f, 0.0f);
            const int FAN_SEGMENTS = 16;
            for (int j = 0; j < FAN_SEGMENTS; j++) {
                glm::vec3 rotated = glm::angleAxis(glm::mix(joint.rotationMin[i], joint.rotationMax[i],
                    (float)j / (FAN_SEGMENTS - 1)), axis) * otherAxis;
                glVertex3f(rotated.x, rotated.y, rotated.z);
            }
            glEnd();
        }
        glPopMatrix();
        
        renderOrientationDirections(position, jointState.combinedRotation, directionSize);
        jointIndex = joint.parentIndex;
        
    } while (jointIndex != -1 && geometry.joints.at(jointIndex).isFree);
    
    glLineWidth(1.0f);
}

