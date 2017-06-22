//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MySkeletonModel.h"

#include <avatars-renderer/Avatar.h>

#include "Application.h"
#include "InterfaceLogging.h"

MySkeletonModel::MySkeletonModel(Avatar* owningAvatar, QObject* parent) : SkeletonModel(owningAvatar, parent) {
}

Rig::CharacterControllerState convertCharacterControllerState(CharacterController::State state) {
    switch (state) {
        default:
        case CharacterController::State::Ground:
            return Rig::CharacterControllerState::Ground;
        case CharacterController::State::Takeoff:
            return Rig::CharacterControllerState::Takeoff;
        case CharacterController::State::InAir:
            return Rig::CharacterControllerState::InAir;
        case CharacterController::State::Hover:
            return Rig::CharacterControllerState::Hover;
    };
}

// Called within Model::simulate call, below.
void MySkeletonModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    const FBXGeometry& geometry = getFBXGeometry();

    Head* head = _owningAvatar->getHead();

    // make sure lookAt is not too close to face (avoid crosseyes)
    glm::vec3 lookAt = head->getLookAtPosition();
    glm::vec3 focusOffset = lookAt - _owningAvatar->getHead()->getEyePosition();
    float focusDistance = glm::length(focusOffset);
    const float MIN_LOOK_AT_FOCUS_DISTANCE = 1.0f;
    if (focusDistance < MIN_LOOK_AT_FOCUS_DISTANCE && focusDistance > EPSILON) {
        lookAt = _owningAvatar->getHead()->getEyePosition() + (MIN_LOOK_AT_FOCUS_DISTANCE / focusDistance) * focusOffset;
    }

    MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);

    Rig::HeadParameters headParams;

    // input action is the highest priority source for head orientation.
    auto avatarHeadPose = myAvatar->getHeadControllerPoseInAvatarFrame();
    if (avatarHeadPose.isValid()) {
        glm::mat4 rigHeadMat = Matrices::Y_180 *
            createMatFromQuatAndPos(avatarHeadPose.getRotation(), avatarHeadPose.getTranslation());
        headParams.rigHeadPosition = extractTranslation(rigHeadMat);
        headParams.rigHeadOrientation = glmExtractRotation(rigHeadMat);
        headParams.headEnabled = true;
    } else {
        // even though full head IK is disabled, the rig still needs the head orientation to rotate the head up and
        // down in desktop mode.
        // preMult 180 is necessary to convert from avatar to rig coordinates.
        // postMult 180 is necessary to convert head from -z forward to z forward.
        headParams.rigHeadOrientation = Quaternions::Y_180 * head->getFinalOrientationInLocalFrame() * Quaternions::Y_180;
        headParams.headEnabled = false;
    }

    auto avatarHipsPose = myAvatar->getHipsControllerPoseInAvatarFrame();
    if (avatarHipsPose.isValid()) {
        glm::mat4 rigHipsMat = Matrices::Y_180 * createMatFromQuatAndPos(avatarHipsPose.getRotation(), avatarHipsPose.getTranslation());
        headParams.hipsMatrix = rigHipsMat;
        headParams.hipsEnabled = true;
    } else {
        headParams.hipsEnabled = false;
    }

    auto avatarSpine2Pose = myAvatar->getSpine2ControllerPoseInAvatarFrame();
    if (avatarSpine2Pose.isValid()) {
        glm::mat4 rigSpine2Mat = Matrices::Y_180 * createMatFromQuatAndPos(avatarSpine2Pose.getRotation(), avatarSpine2Pose.getTranslation());
        headParams.spine2Matrix = rigSpine2Mat;
        headParams.spine2Enabled = true;
    } else {
        headParams.spine2Enabled = false;
    }

    auto avatarRightArmPose = myAvatar->getRightArmControllerPoseInAvatarFrame();
    if (avatarRightArmPose.isValid()) {
        glm::mat4 rightArmMat = Matrices::Y_180 * createMatFromQuatAndPos(avatarRightArmPose.getRotation(), avatarRightArmPose.getTranslation());
        headParams.rightArmPosition = extractTranslation(rightArmMat);
        headParams.rightArmRotation = glmExtractRotation(rightArmMat);
        headParams.rightArmEnabled = true;
    } else {
        headParams.rightArmEnabled = false;
    }
    
    auto avatarLeftArmPose = myAvatar->getLeftArmControllerPoseInAvatarFrame();
    if (avatarLeftArmPose.isValid()) {
        glm::mat4 leftArmMat = Matrices::Y_180 * createMatFromQuatAndPos(avatarLeftArmPose.getRotation(), avatarLeftArmPose.getTranslation());
        headParams.leftArmPosition = extractTranslation(leftArmMat);
        headParams.leftArmRotation = glmExtractRotation(leftArmMat);
        headParams.leftArmEnabled = true;
    } else {
        headParams.leftArmEnabled = false;
    }

    headParams.isTalking = head->getTimeWithoutTalking() <= 1.5f;

    _rig.updateFromHeadParameters(headParams, deltaTime);

    Rig::HandAndFeetParameters handAndFeetParams;

    auto leftPose = myAvatar->getLeftHandControllerPoseInAvatarFrame();
    if (leftPose.isValid()) {
        handAndFeetParams.isLeftEnabled = true;
        handAndFeetParams.leftPosition = Quaternions::Y_180 * leftPose.getTranslation();
        handAndFeetParams.leftOrientation = Quaternions::Y_180 * leftPose.getRotation();
    } else {
        handAndFeetParams.isLeftEnabled = false;
    }

    auto rightPose = myAvatar->getRightHandControllerPoseInAvatarFrame();
    if (rightPose.isValid()) {
        handAndFeetParams.isRightEnabled = true;
        handAndFeetParams.rightPosition = Quaternions::Y_180 * rightPose.getTranslation();
        handAndFeetParams.rightOrientation = Quaternions::Y_180 * rightPose.getRotation();
    } else {
        handAndFeetParams.isRightEnabled = false;
    }

    auto leftFootPose = myAvatar->getLeftFootControllerPoseInAvatarFrame();
    if (leftFootPose.isValid()) {
        handAndFeetParams.isLeftFootEnabled = true;
        handAndFeetParams.leftFootPosition = Quaternions::Y_180 * leftFootPose.getTranslation();
        handAndFeetParams.leftFootOrientation = Quaternions::Y_180 * leftFootPose.getRotation();
    } else {
        handAndFeetParams.isLeftFootEnabled = false;
    }

    auto rightFootPose = myAvatar->getRightFootControllerPoseInAvatarFrame();
    if (rightFootPose.isValid()) {
        handAndFeetParams.isRightFootEnabled = true;
        handAndFeetParams.rightFootPosition = Quaternions::Y_180 * rightFootPose.getTranslation();
        handAndFeetParams.rightFootOrientation = Quaternions::Y_180 * rightFootPose.getRotation();
    } else {
        handAndFeetParams.isRightFootEnabled = false;
    }

    handAndFeetParams.bodyCapsuleRadius = myAvatar->getCharacterController()->getCapsuleRadius();
    handAndFeetParams.bodyCapsuleHalfHeight = myAvatar->getCharacterController()->getCapsuleHalfHeight();
    handAndFeetParams.bodyCapsuleLocalOffset = myAvatar->getCharacterController()->getCapsuleLocalOffset();

    _rig.updateFromHandAndFeetParameters(handAndFeetParams, deltaTime);

    Rig::CharacterControllerState ccState = convertCharacterControllerState(myAvatar->getCharacterController()->getState());

    auto velocity = myAvatar->getLocalVelocity();
    auto position = myAvatar->getLocalPosition();
    auto orientation = myAvatar->getLocalOrientation();
    _rig.computeMotionAnimationState(deltaTime, position, velocity, orientation, ccState);

    // evaluate AnimGraph animation and update jointStates.
    Model::updateRig(deltaTime, parentTransform);

    Rig::EyeParameters eyeParams;
    eyeParams.eyeLookAt = lookAt;
    eyeParams.eyeSaccade = head->getSaccade();
    eyeParams.modelRotation = getRotation();
    eyeParams.modelTranslation = getTranslation();
    eyeParams.leftEyeJointIndex = geometry.leftEyeJointIndex;
    eyeParams.rightEyeJointIndex = geometry.rightEyeJointIndex;

    _rig.updateFromEyeParameters(eyeParams);

    auto fingerPoses = myAvatar->getLeftHandFingerControllerPosesInSensorFrame();
    if (leftHandPose.isValid() && fingerPoses.size() > 0) {
        // Can just check the first finger pose because either all finger poses will be valid or none of them will.
        if (fingerPoses[(int)controller::Action::LEFT_HAND_INDEX1].isValid()) {
            glm::quat handJointRotation = myAvatar->getLeftHandControllerPoseInSensorFrame().getRotation();

            glm::quat previousJointRotation;
            glm::quat thisJointRotation;

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_THUMB1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandThumb1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_THUMB2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandThumb2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_THUMB3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandThumb3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_THUMB4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandThumb4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_INDEX1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandIndex1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_INDEX2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandIndex2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_INDEX3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandIndex3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_INDEX4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandIndex4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_MIDDLE1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandMiddle1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_MIDDLE2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandMiddle2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_MIDDLE3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandMiddle3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_MIDDLE4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandMiddle4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_RING1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandRing1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_RING2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandRing2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_RING3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandRing3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_RING4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandRing4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_PINKY1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandPinky1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_PINKY2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandPinky2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_PINKY3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandPinky3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::LEFT_HAND_PINKY4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("LeftHandPinky4"), glm::inverse(previousJointRotation) * thisJointRotation);
        } else {
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandThumb1"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandThumb2"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandThumb3"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandThumb4"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandIndex1"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandIndex2"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandIndex3"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandIndex4"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandMiddle1"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandMiddle2"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandMiddle3"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandMiddle4"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandRing1"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandRing2"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandRing3"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandRing4"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandPinky1"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandPinky2"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandPinky3"));
            myAvatar->clearJointData(_rig.indexOfJoint("LeftHandPinky4"));
        }
    }

    fingerPoses = myAvatar->getRightHandFingerControllerPosesInSensorFrame();
    if (rightHandPose.isValid() && fingerPoses.size() > 0) {
        // Can just check the first finger pose because either all finger poses will be valid or none of them will.
        if (fingerPoses[(int)controller::Action::RIGHT_HAND_INDEX1].isValid()) {
            glm::quat handJointRotation = myAvatar->getRightHandControllerPoseInSensorFrame().getRotation();

            glm::quat previousJointRotation;
            glm::quat thisJointRotation;

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_THUMB1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandThumb1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_THUMB2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandThumb2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_THUMB3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandThumb3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_THUMB4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandThumb4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_INDEX1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandIndex1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_INDEX2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandIndex2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_INDEX3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandIndex3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_INDEX4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandIndex4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_MIDDLE1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandMiddle1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_MIDDLE2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandMiddle2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_MIDDLE3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandMiddle3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_MIDDLE4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandMiddle4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_RING1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandRing1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_RING2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandRing2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_RING3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandRing3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_RING4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandRing4"), glm::inverse(previousJointRotation) * thisJointRotation);

            previousJointRotation = handJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_PINKY1].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandPinky1"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_PINKY2].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandPinky2"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_PINKY3].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandPinky3"), glm::inverse(previousJointRotation) * thisJointRotation);
            previousJointRotation = thisJointRotation;
            thisJointRotation = fingerPoses[(int)controller::Action::RIGHT_HAND_PINKY4].getRotation();
            myAvatar->setJointRotation(_rig.indexOfJoint("RightHandPinky4"), glm::inverse(previousJointRotation) * thisJointRotation);
        } else {
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandThumb1"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandThumb2"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandThumb3"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandThumb4"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandIndex1"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandIndex2"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandIndex3"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandIndex4"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandMiddle1"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandMiddle2"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandMiddle3"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandMiddle4"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandRing1"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandRing2"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandRing3"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandRing4"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandPinky1"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandPinky2"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandPinky3"));
            myAvatar->clearJointData(_rig.indexOfJoint("RightHandPinky4"));
        }
    }
}
