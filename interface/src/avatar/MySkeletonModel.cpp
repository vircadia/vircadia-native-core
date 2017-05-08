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

MySkeletonModel::MySkeletonModel(Avatar* owningAvatar, QObject* parent, RigPointer rig) : SkeletonModel(owningAvatar, parent, rig) {
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
    glm::vec3 lookAt = _owningAvatar->isMyAvatar() ?  head->getLookAtPosition() : head->getCorrectedLookAtPosition();
    MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);

    Rig::HeadParameters headParams;

    // input action is the highest priority source for head orientation.
    auto avatarHeadPose = myAvatar->getHeadControllerPoseInAvatarFrame();
    if (avatarHeadPose.isValid()) {
        glm::mat4 rigHeadMat = Matrices::Y_180 * createMatFromQuatAndPos(avatarHeadPose.getRotation(), avatarHeadPose.getTranslation());
        headParams.rigHeadPosition = extractTranslation(rigHeadMat);
        headParams.rigHeadOrientation = glmExtractRotation(rigHeadMat);
        headParams.headEnabled = true;
    } else {
        if (qApp->isHMDMode()) {
            // get HMD position from sensor space into world space, and back into rig space
            glm::mat4 worldHMDMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
            glm::mat4 rigToWorld = createMatFromQuatAndPos(getRotation(), getTranslation());
            glm::mat4 worldToRig = glm::inverse(rigToWorld);
            glm::mat4 rigHMDMat = worldToRig * worldHMDMat;
            _rig->computeHeadFromHMD(AnimPose(rigHMDMat), headParams.rigHeadPosition, headParams.rigHeadOrientation);
            headParams.headEnabled = true;
        } else {
            // even though full head IK is disabled, the rig still needs the head orientation to rotate the head up and down in desktop mode.
            // preMult 180 is necessary to convert from avatar to rig coordinates.
            // postMult 180 is necessary to convert head from -z forward to z forward.
            headParams.rigHeadOrientation = Quaternions::Y_180 * head->getFinalOrientationInLocalFrame() * Quaternions::Y_180;
            headParams.headEnabled = false;
        }
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

    headParams.isTalking = head->getTimeWithoutTalking() <= 1.5f;

    _rig->updateFromHeadParameters(headParams, deltaTime);

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

    _rig->updateFromHandAndFeetParameters(handAndFeetParams, deltaTime);

    Rig::CharacterControllerState ccState = convertCharacterControllerState(myAvatar->getCharacterController()->getState());

    auto velocity = myAvatar->getLocalVelocity();
    auto position = myAvatar->getLocalPosition();
    auto orientation = myAvatar->getLocalOrientation();
    _rig->computeMotionAnimationState(deltaTime, position, velocity, orientation, ccState);

    Rig::EyeParameters eyeParams;
    eyeParams.eyeLookAt = lookAt;
    eyeParams.eyeSaccade = head->getSaccade();
    eyeParams.modelRotation = getRotation();
    eyeParams.modelTranslation = getTranslation();
    eyeParams.leftEyeJointIndex = geometry.leftEyeJointIndex;
    eyeParams.rightEyeJointIndex = geometry.rightEyeJointIndex;

    _rig->updateFromEyeParameters(eyeParams);

    // evaluate AnimGraph animation and update jointStates.
    Parent::updateRig(deltaTime, parentTransform);
}

