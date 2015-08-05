//
//  AvatarRig.cpp
//  libraries/animation/src/
//
//  Created by SethAlves on 2015-7-22.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarRig.h"

/// Updates the state of the joint at the specified index.
void AvatarRig::updateJointState(int index, glm::mat4 parentTransform) {
    if (index < 0 && index >= _jointStates.size()) {
        return; // bail
    }
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();

    // compute model transforms
    if (index == _rootJointIndex) {
        // we always zero-out the translation part of an avatar's root join-transform.
        state.computeTransform(parentTransform);
        clearJointTransformTranslation(index);
    } else {
        // guard against out-of-bounds access to _jointStates
        int parentIndex = joint.parentIndex;
        if (parentIndex >= 0 && parentIndex < _jointStates.size()) {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.computeTransform(parentState.getTransform(), parentState.getTransformChanged());
        }
    }
}

void AvatarRig::setHandPosition(int jointIndex,
                                const glm::vec3& position, const glm::quat& rotation,
                                float scale, float priority) {
    bool rightHand = (jointIndex == _rightHandJointIndex);

    int elbowJointIndex = rightHand ? _rightElbowJointIndex : _leftElbowJointIndex;
    int shoulderJointIndex = rightHand ? _rightShoulderJointIndex : _leftShoulderJointIndex;

    // this algorithm is from sample code from sixense
    if (elbowJointIndex == -1 || shoulderJointIndex == -1) {
        return;
    }

    glm::vec3 shoulderPosition;
    if (!getJointPosition(shoulderJointIndex, shoulderPosition)) {
        return;
    }

    // precomputed lengths
    float upperArmLength = _jointStates[elbowJointIndex].getFBXJoint().distanceToParent * scale;
    float lowerArmLength = _jointStates[jointIndex].getFBXJoint().distanceToParent * scale;

    // first set wrist position
    glm::vec3 wristPosition = position;

    glm::vec3 shoulderToWrist = wristPosition - shoulderPosition;
    float distanceToWrist = glm::length(shoulderToWrist);

    // prevent gimbal lock
    if (distanceToWrist > upperArmLength + lowerArmLength - EPSILON) {
        distanceToWrist = upperArmLength + lowerArmLength - EPSILON;
        shoulderToWrist = glm::normalize(shoulderToWrist) * distanceToWrist;
        wristPosition = shoulderPosition + shoulderToWrist;
    }

    // cosine of angle from upper arm to hand vector
    float cosA = (upperArmLength * upperArmLength + distanceToWrist * distanceToWrist - lowerArmLength * lowerArmLength) /
        (2 * upperArmLength * distanceToWrist);
    float mid = upperArmLength * cosA;
    float height = sqrt(upperArmLength * upperArmLength + mid * mid - 2 * upperArmLength * mid * cosA);

    // direction of the elbow
    glm::vec3 handNormal = glm::cross(rotation * glm::vec3(0.0f, 1.0f, 0.0f), shoulderToWrist); // elbow rotating with wrist
    glm::vec3 relaxedNormal = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), shoulderToWrist); // elbow pointing straight down
    const float NORMAL_WEIGHT = 0.5f;
    glm::vec3 finalNormal = glm::mix(relaxedNormal, handNormal, NORMAL_WEIGHT);

    if (rightHand ? (finalNormal.y > 0.0f) : (finalNormal.y < 0.0f)) {
        finalNormal.y = 0.0f; // dont allow elbows to point inward (y is vertical axis)
    }

    glm::vec3 tangent = glm::normalize(glm::cross(shoulderToWrist, finalNormal));

    // ik solution
    glm::vec3 elbowPosition = shoulderPosition + glm::normalize(shoulderToWrist) * mid - tangent * height;
    glm::vec3 forwardVector(rightHand ? -1.0f : 1.0f, 0.0f, 0.0f);
    glm::quat shoulderRotation = rotationBetween(forwardVector, elbowPosition - shoulderPosition);

    setJointRotationInBindFrame(shoulderJointIndex, shoulderRotation, priority);
    setJointRotationInBindFrame(elbowJointIndex,
                                rotationBetween(shoulderRotation * forwardVector, wristPosition - elbowPosition) *
                                shoulderRotation, priority);
    setJointRotationInBindFrame(jointIndex, rotation, priority);
}
