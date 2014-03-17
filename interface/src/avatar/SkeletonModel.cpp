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

    // find the left and rightmost active Leap palms
    int leftPalmIndex, rightPalmIndex;
    Hand* hand = _owningAvatar->getHand();
    hand->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);

    const float HAND_RESTORATION_PERIOD = 1.f;  // seconds
    float handRestorePercent = glm::clamp(deltaTime / HAND_RESTORATION_PERIOD, 0.f, 1.f);

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (leftPalmIndex == -1) {
        // no Leap data; set hands from mouse
        if (_owningAvatar->getHandState() == HAND_STATE_NULL) {
            restoreRightHandPosition(handRestorePercent);
        } else {
            applyHandPosition(geometry.rightHandJointIndex, _owningAvatar->getHandPosition());
        }
        restoreLeftHandPosition(handRestorePercent);

    } else if (leftPalmIndex == rightPalmIndex) {
        // right hand only
        applyPalmData(geometry.rightHandJointIndex, geometry.rightFingerJointIndices, geometry.rightFingertipJointIndices,
            hand->getPalms()[leftPalmIndex]);
        restoreLeftHandPosition(handRestorePercent);

    } else {
        applyPalmData(geometry.leftHandJointIndex, geometry.leftFingerJointIndices, geometry.leftFingertipJointIndices,
            hand->getPalms()[leftPalmIndex]);
        applyPalmData(geometry.rightHandJointIndex, geometry.rightFingerJointIndices, geometry.rightFingertipJointIndices,
            hand->getPalms()[rightPalmIndex]);
    }
}

bool SkeletonModel::render(float alpha) {

    if (_jointStates.isEmpty()) {
        return false;
    }

    Model::render(alpha);

    return true;
}

void SkeletonModel::getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const {
    if (jointIndex == -1) {
        return;
    }
    if (jointIndex == getLeftHandJointIndex()
        || jointIndex == getRightHandJointIndex()) {
        // TODO: also add fingers and other hand-parts
        shapes.push_back(_shapes[jointIndex]);
    }
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
    applyRotationDelta(jointIndex, rotationBetween(handRotation * glm::vec3(-sign, 0.0f, 0.0f), forearmVector), false);
}

void SkeletonModel::applyPalmData(int jointIndex, const QVector<int>& fingerJointIndices,
        const QVector<int>& fingertipJointIndices, PalmData& palm) {
    if (jointIndex == -1) {
        return;
    }
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
    for (size_t i = 0; i < palm.getNumFingers(); i++) {
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
    const unsigned int MIN_ROTATION_FINGERS = 3;
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
    if (!_owningAvatar->isMyAvatar()) {
        return;
    }
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(_rotation);
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.transform * glm::translate(state.translation) * 
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation)));
    state.rotation = glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getLeanSideways(), 
        glm::normalize(inverse * axes[2])) * glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getLeanForward(), 
        glm::normalize(inverse * axes[0])) * joint.rotation;
}

