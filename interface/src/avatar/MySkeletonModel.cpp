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
    assert(myAvatar);

    Rig::ControllerParameters params;

    AnimPose avatarToRigPose(glm::vec3(1.0f), Quaternions::Y_180, glm::vec3(0.0f));

    // input action is the highest priority source for head orientation.
    auto avatarHeadPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::HEAD);
    if (avatarHeadPose.isValid()) {
        AnimPose pose(avatarHeadPose.getRotation(), avatarHeadPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_Head] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_Head] = true;
    } else {
        // even though full head IK is disabled, the rig still needs the head orientation to rotate the head up and
        // down in desktop mode.
        // preMult 180 is necessary to convert from avatar to rig coordinates.
        // postMult 180 is necessary to convert head from -z forward to z forward.
        glm::quat headRot = Quaternions::Y_180 * head->getFinalOrientationInLocalFrame() * Quaternions::Y_180;
        params.controllerPoses[Rig::ControllerType_Head] = AnimPose(glm::vec3(1.0f), headRot, glm::vec3(0.0f));
        params.controllerActiveFlags[Rig::ControllerType_Head] = false;
    }

    auto avatarHipsPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::HIPS);
    if (avatarHipsPose.isValid()) {
        AnimPose pose(avatarHipsPose.getRotation(), avatarHipsPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_Hips] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_Hips] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_Hips] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_Hips] = false;
    }

    auto avatarSpine2Pose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::SPINE2);
    if (avatarSpine2Pose.isValid()) {
        AnimPose pose(avatarSpine2Pose.getRotation(), avatarSpine2Pose.getTranslation());
        params.controllerPoses[Rig::ControllerType_Spine2] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_Spine2] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_Spine2] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_Spine2] = false;
    }

    auto avatarRightArmPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::RIGHT_ARM);
    if (avatarRightArmPose.isValid()) {
        AnimPose pose(avatarRightArmPose.getRotation(), avatarRightArmPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_RightArm] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_RightArm] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_RightArm] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_RightArm] = false;
    }

    auto avatarLeftArmPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::LEFT_ARM);
    if (avatarLeftArmPose.isValid()) {
        AnimPose pose(avatarLeftArmPose.getRotation(), avatarLeftArmPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_LeftArm] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_LeftArm] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_LeftArm] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_LeftArm] = false;
    }

    auto avatarLeftHandPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND);
    if (avatarLeftHandPose.isValid()) {
        AnimPose pose(avatarLeftHandPose.getRotation(), avatarLeftHandPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_LeftHand] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_LeftHand] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_LeftHand] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_LeftHand] = false;
    }

    auto avatarRightHandPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND);
    if (avatarRightHandPose.isValid()) {
        AnimPose pose(avatarRightHandPose.getRotation(), avatarRightHandPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_RightHand] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_RightHand] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_RightHand] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_RightHand] = false;
    }

    auto avatarLeftFootPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::LEFT_FOOT);
    if (avatarLeftFootPose.isValid()) {
        AnimPose pose(avatarLeftFootPose.getRotation(), avatarLeftFootPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_LeftFoot] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_LeftFoot] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_LeftFoot] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_LeftFoot] = false;
    }

    auto avatarRightFootPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::RIGHT_FOOT);
    if (avatarRightFootPose.isValid()) {
        AnimPose pose(avatarRightFootPose.getRotation(), avatarRightFootPose.getTranslation());
        params.controllerPoses[Rig::ControllerType_RightFoot] = avatarToRigPose * pose;
        params.controllerActiveFlags[Rig::ControllerType_RightFoot] = true;
    } else {
        params.controllerPoses[Rig::ControllerType_RightFoot] = AnimPose::identity;
        params.controllerActiveFlags[Rig::ControllerType_RightFoot] = false;
    }

    params.bodyCapsuleRadius = myAvatar->getCharacterController()->getCapsuleRadius();
    params.bodyCapsuleHalfHeight = myAvatar->getCharacterController()->getCapsuleHalfHeight();
    params.bodyCapsuleLocalOffset = myAvatar->getCharacterController()->getCapsuleLocalOffset();

    params.isTalking = head->getTimeWithoutTalking() <= 1.5f;

    _rig.updateFromControllerParameters(params, deltaTime);

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

    updateFingers();
}


void MySkeletonModel::updateFingers() {

    MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);

    static std::vector<std::vector<std::pair<controller::Action, QString>>> fingerChains = {
        {
            { controller::Action::LEFT_HAND, "LeftHand" },
            { controller::Action::LEFT_HAND_THUMB1, "LeftHandThumb1" },
            { controller::Action::LEFT_HAND_THUMB2, "LeftHandThumb2" },
            { controller::Action::LEFT_HAND_THUMB3, "LeftHandThumb3" },
            { controller::Action::LEFT_HAND_THUMB4, "LeftHandThumb4" }
        },
        {
            { controller::Action::LEFT_HAND, "LeftHand" },
            { controller::Action::LEFT_HAND_INDEX1, "LeftHandIndex1" },
            { controller::Action::LEFT_HAND_INDEX2, "LeftHandIndex2" },
            { controller::Action::LEFT_HAND_INDEX3, "LeftHandIndex3" },
            { controller::Action::LEFT_HAND_INDEX4, "LeftHandIndex4" }
        },
        {
            { controller::Action::LEFT_HAND, "LeftHand" },
            { controller::Action::LEFT_HAND_MIDDLE1, "LeftHandMiddle1" },
            { controller::Action::LEFT_HAND_MIDDLE2, "LeftHandMiddle2" },
            { controller::Action::LEFT_HAND_MIDDLE3, "LeftHandMiddle3" },
            { controller::Action::LEFT_HAND_MIDDLE4, "LeftHandMiddle4" }
        },
        {
            { controller::Action::LEFT_HAND, "LeftHand" },
            { controller::Action::LEFT_HAND_RING1, "LeftHandRing1" },
            { controller::Action::LEFT_HAND_RING2, "LeftHandRing2" },
            { controller::Action::LEFT_HAND_RING3, "LeftHandRing3" },
            { controller::Action::LEFT_HAND_RING4, "LeftHandRing4" }
        },
        {
            { controller::Action::LEFT_HAND, "LeftHand" },
            { controller::Action::LEFT_HAND_PINKY1, "LeftHandPinky1" },
            { controller::Action::LEFT_HAND_PINKY2, "LeftHandPinky2" },
            { controller::Action::LEFT_HAND_PINKY3, "LeftHandPinky3" },
            { controller::Action::LEFT_HAND_PINKY4, "LeftHandPinky4" }
        },
        {
            { controller::Action::RIGHT_HAND, "RightHand" },
            { controller::Action::RIGHT_HAND_THUMB1, "RightHandThumb1" },
            { controller::Action::RIGHT_HAND_THUMB2, "RightHandThumb2" },
            { controller::Action::RIGHT_HAND_THUMB3, "RightHandThumb3" },
            { controller::Action::RIGHT_HAND_THUMB4, "RightHandThumb4" }
        },
        {
            { controller::Action::RIGHT_HAND, "RightHand" },
            { controller::Action::RIGHT_HAND_INDEX1, "RightHandIndex1" },
            { controller::Action::RIGHT_HAND_INDEX2, "RightHandIndex2" },
            { controller::Action::RIGHT_HAND_INDEX3, "RightHandIndex3" },
            { controller::Action::RIGHT_HAND_INDEX4, "RightHandIndex4" }
        },
        {
            { controller::Action::RIGHT_HAND, "RightHand" },
            { controller::Action::RIGHT_HAND_MIDDLE1, "RightHandMiddle1" },
            { controller::Action::RIGHT_HAND_MIDDLE2, "RightHandMiddle2" },
            { controller::Action::RIGHT_HAND_MIDDLE3, "RightHandMiddle3" },
            { controller::Action::RIGHT_HAND_MIDDLE4, "RightHandMiddle4" }
        },
        {
            { controller::Action::RIGHT_HAND, "RightHand" },
            { controller::Action::RIGHT_HAND_RING1, "RightHandRing1" },
            { controller::Action::RIGHT_HAND_RING2, "RightHandRing2" },
            { controller::Action::RIGHT_HAND_RING3, "RightHandRing3" },
            { controller::Action::RIGHT_HAND_RING4, "RightHandRing4" }
        },
        {
            { controller::Action::RIGHT_HAND, "RightHand" },
            { controller::Action::RIGHT_HAND_PINKY1, "RightHandPinky1" },
            { controller::Action::RIGHT_HAND_PINKY2, "RightHandPinky2" },
            { controller::Action::RIGHT_HAND_PINKY3, "RightHandPinky3" },
            { controller::Action::RIGHT_HAND_PINKY4, "RightHandPinky4" }
        }
    };

    const float CONTROLLER_PRIORITY = 2.0f;

    for (auto& chain : fingerChains) {
        glm::quat prevAbsRot = Quaternions::IDENTITY;
        for (auto& link : chain) {
            int index = _rig.indexOfJoint(link.second);
            if (index >= 0) {
                auto pose = myAvatar->getControllerPoseInSensorFrame(link.first);
                if (pose.valid) {
                    glm::quat relRot = glm::inverse(prevAbsRot) * pose.getRotation();
                    // only set the rotation for the finger joints, not the hands.
                    if (link.first != controller::Action::LEFT_HAND && link.first != controller::Action::RIGHT_HAND) {
                        _rig.setJointRotation(index, true, relRot, CONTROLLER_PRIORITY);
                    }
                    prevAbsRot = pose.getRotation();
                } else {
                    _rig.clearJointAnimationPriority(index);
                }
            }
        }
    }
}
