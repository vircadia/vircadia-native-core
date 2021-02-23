//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MySkeletonModel.h"

#include <avatars-renderer/Avatar.h>
#include <DebugDraw.h>
#include <CubicHermiteSpline.h>

#include "Application.h"
#include "InterfaceLogging.h"
#include "AnimUtil.h"



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
        case CharacterController::State::Seated:
            return Rig::CharacterControllerState::Seated;
    };
}

#if defined(Q_OS_ANDROID) || defined(HIFI_USE_OPTIMIZED_IK)
static glm::vec3 computeSpine2WithHeadHipsSpline(MyAvatar* myAvatar, AnimPose hipsIKTargetPose, AnimPose headIKTargetPose) {

    // the the ik targets to compute the spline with
    CubicHermiteSplineFunctorWithArcLength splineFinal(headIKTargetPose.rot(), headIKTargetPose.trans(), hipsIKTargetPose.rot(), hipsIKTargetPose.trans());

    // measure the total arc length along the spline
    float totalArcLength = splineFinal.arcLength(1.0f);
    float tFinal = splineFinal.arcLengthInverse(myAvatar->getSpine2SplineRatio() * totalArcLength);
    glm::vec3 spine2Translation = splineFinal(tFinal);

    return spine2Translation + myAvatar->getSpine2SplineOffset();

}
#endif

static AnimPose computeHipsInSensorFrame(MyAvatar* myAvatar, bool isFlying) {
    glm::mat4 worldToSensorMat = glm::inverse(myAvatar->getSensorToWorldMatrix());

    // check for pinned hips.
    auto hipsIndex = myAvatar->getJointIndex("Hips");
    if (myAvatar->isJointPinned(hipsIndex)) {
        Transform avatarTransform = myAvatar->getTransform();
        AnimPose result = AnimPose(worldToSensorMat * avatarTransform.getMatrix() * Matrices::Y_180);
        result.scale() = glm::vec3(1.0f, 1.0f, 1.0f);
        return result;
    }

    // Use the center-of-gravity model if the user and the avatar are standing, unless flying or walking.
    // If artificial standing is disabled, use center-of-gravity regardless of the user's sit/stand state.
    bool useCenterOfGravityModel =
        myAvatar->getCenterOfGravityModelEnabled() && !isFlying && !myAvatar->getIsInWalkingState() &&
        (!myAvatar->getHMDCrouchRecenterEnabled() || !myAvatar->getIsInSittingState()) &&
        myAvatar->getHMDLeanRecenterEnabled() &&
        (myAvatar->getAllowAvatarLeaningPreference() != MyAvatar::AllowAvatarLeaningPreference::AlwaysNoRecenter);

    glm::mat4 hipsMat;
    if (useCenterOfGravityModel) {
        // then we use center of gravity model
        hipsMat = myAvatar->deriveBodyUsingCgModel();
    } else {
        // otherwise use the default of putting the hips under the head
        hipsMat = myAvatar->deriveBodyFromHMDSensor(true);
    }
    glm::vec3 hipsPos = extractTranslation(hipsMat);
    glm::quat hipsRot = glmExtractRotation(hipsMat);


    glm::mat4 avatarToWorldMat = myAvatar->getTransform().getMatrix();
    glm::mat4 avatarToSensorMat = worldToSensorMat * avatarToWorldMat;

    // dampen hips rotation, by mixing it with the avatar orientation in sensor space
    // turning this off for center of gravity model because it is already mixed in there
    if (!useCenterOfGravityModel) {
        const float MIX_RATIO = 0.5f;
        hipsRot = safeLerp(glmExtractRotation(avatarToSensorMat), hipsRot, MIX_RATIO);
    }

    if (isFlying) {
        // rotate the hips back to match the flying animation.

        const float TILT_ANGLE = 0.523f;
        const glm::quat tiltRot = glm::angleAxis(TILT_ANGLE, glm::normalize(transformVectorFast(avatarToSensorMat, -Vectors::UNIT_X)));

        glm::vec3 headPos;
        int headIndex = myAvatar->getJointIndex("Head");
        if (headIndex != -1) {
            headPos = transformPoint(avatarToSensorMat, myAvatar->getAbsoluteJointTranslationInObjectFrame(headIndex));
        } else {
            headPos = transformPoint(myAvatar->getSensorToWorldMatrix(), myAvatar->getHMDSensorPosition());
        }
        hipsRot = tiltRot * hipsRot;
        hipsPos = headPos + tiltRot * (hipsPos - headPos);
    }

    // AJT: TODO can we remove this?
    return AnimPose(hipsRot * Quaternions::Y_180, hipsPos);
}

// Called within Model::simulate call, below.
void MySkeletonModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    const HFMModel& hfmModel = getHFMModel();

    MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);
    assert(myAvatar);

    Head* head = _owningAvatar->getHead();

    bool eyePosesValid = (myAvatar->getControllerPoseInSensorFrame(controller::Action::LEFT_EYE).isValid() ||
                          myAvatar->getControllerPoseInSensorFrame(controller::Action::RIGHT_EYE).isValid());
    glm::vec3 lookAt;
    if (eyePosesValid) {
        lookAt = head->getLookAtPosition(); // don't apply no-crosseyes code when eyes are being tracked
    } else {
        lookAt = avoidCrossedEyes(head->getLookAtPosition());
    }

    Rig::ControllerParameters params;

    AnimPose avatarToRigPose(glm::vec3(1.0f), Quaternions::Y_180, glm::vec3(0.0f));

    glm::mat4 rigToAvatarMatrix = Matrices::Y_180;
    glm::mat4 avatarToWorldMatrix = createMatFromQuatAndPos(myAvatar->getWorldOrientation(), myAvatar->getWorldPosition());
    glm::mat4 sensorToWorldMatrix = myAvatar->getSensorToWorldMatrix();
    params.rigToSensorMatrix = glm::inverse(sensorToWorldMatrix) * avatarToWorldMatrix * rigToAvatarMatrix;

    // input action is the highest priority source for head orientation.
    auto avatarHeadPose = myAvatar->getControllerPoseInAvatarFrame(controller::Action::HEAD);
    if (avatarHeadPose.isValid()) {
        AnimPose pose(avatarHeadPose.getRotation(), avatarHeadPose.getTranslation());
        params.primaryControllerPoses[Rig::PrimaryControllerType_Head] = avatarToRigPose * pose;
        params.primaryControllerFlags[Rig::PrimaryControllerType_Head] = (uint8_t)Rig::ControllerFlags::Enabled;
    } else {
        // even though full head IK is disabled, the rig still needs the head orientation to rotate the head up and
        // down in desktop mode.
        // preMult 180 is necessary to convert from avatar to rig coordinates.
        // postMult 180 is necessary to convert head from -z forward to z forward.
        glm::quat headRot = Quaternions::Y_180 * head->getFinalOrientationInLocalFrame() * Quaternions::Y_180;
        params.primaryControllerPoses[Rig::PrimaryControllerType_Head] = AnimPose(glm::vec3(1.0f), headRot, glm::vec3(0.0f));
        params.primaryControllerFlags[Rig::PrimaryControllerType_Head] = 0;
    }

    //
    // primary controller poses, control IK targets directly.
    //

    static const std::vector<std::pair<controller::Action, Rig::PrimaryControllerType>> primaryControllers = {
        { controller::Action::LEFT_HAND, Rig::PrimaryControllerType_LeftHand },
        { controller::Action::RIGHT_HAND, Rig::PrimaryControllerType_RightHand },
        { controller::Action::HIPS, Rig::PrimaryControllerType_Hips },
        { controller::Action::LEFT_FOOT, Rig::PrimaryControllerType_LeftFoot },
        { controller::Action::RIGHT_FOOT, Rig::PrimaryControllerType_RightFoot },
        { controller::Action::SPINE2, Rig::PrimaryControllerType_Spine2 }
    };

    for (auto pair : primaryControllers) {
        auto controllerPose = myAvatar->getControllerPoseInAvatarFrame(pair.first);
        if (controllerPose.isValid()) {
            AnimPose pose(controllerPose.getRotation(), controllerPose.getTranslation());
            params.primaryControllerPoses[pair.second] = avatarToRigPose * pose;
            params.primaryControllerFlags[pair.second] = (uint8_t)Rig::ControllerFlags::Enabled;
        } else {
            params.primaryControllerPoses[pair.second] = AnimPose::identity;
            params.primaryControllerFlags[pair.second] = 0;
        }
    }

    //
    // secondary controller poses, influence the pose of the skeleton indirectly.
    //

    static const std::vector<std::pair<controller::Action, Rig::SecondaryControllerType>> secondaryControllers = {
        { controller::Action::LEFT_SHOULDER, Rig::SecondaryControllerType_LeftShoulder },
        { controller::Action::RIGHT_SHOULDER, Rig::SecondaryControllerType_RightShoulder },
        { controller::Action::LEFT_ARM, Rig::SecondaryControllerType_LeftArm },
        { controller::Action::RIGHT_ARM, Rig::SecondaryControllerType_RightArm },
        { controller::Action::LEFT_FORE_ARM, Rig::SecondaryControllerType_LeftForeArm },
        { controller::Action::RIGHT_FORE_ARM, Rig::SecondaryControllerType_RightForeArm },
        { controller::Action::LEFT_UP_LEG, Rig::SecondaryControllerType_LeftUpLeg },
        { controller::Action::RIGHT_UP_LEG, Rig::SecondaryControllerType_RightUpLeg },
        { controller::Action::LEFT_LEG, Rig::SecondaryControllerType_LeftLeg },
        { controller::Action::RIGHT_LEG, Rig::SecondaryControllerType_RightLeg },
        { controller::Action::LEFT_TOE_BASE, Rig::SecondaryControllerType_LeftToeBase },
        { controller::Action::RIGHT_TOE_BASE, Rig::SecondaryControllerType_RightToeBase }
    };

    for (auto pair : secondaryControllers) {
        auto controllerPose = myAvatar->getControllerPoseInAvatarFrame(pair.first);
        if (controllerPose.isValid()) {
            AnimPose pose(controllerPose.getRotation(), controllerPose.getTranslation());
            params.secondaryControllerPoses[pair.second] = avatarToRigPose * pose;
            params.secondaryControllerFlags[pair.second] = (uint8_t)Rig::ControllerFlags::Enabled;
        } else {
            params.secondaryControllerPoses[pair.second] = AnimPose::identity;
            params.secondaryControllerFlags[pair.second] = 0;
        }
    }

    bool isFlying = (myAvatar->getCharacterController()->getState() == CharacterController::State::Hover || myAvatar->getCharacterController()->computeCollisionMask() == BULLET_COLLISION_MASK_COLLISIONLESS);
    if (isFlying != _prevIsFlying) {
        const float FLY_TO_IDLE_HIPS_TRANSITION_TIME = 0.5f;
        _flyIdleTimer = FLY_TO_IDLE_HIPS_TRANSITION_TIME;
    } else {
        _flyIdleTimer -= deltaTime;
    }
    _prevIsFlying = isFlying;

    // if hips are not under direct control, estimate the hips position.
    if (avatarHeadPose.isValid() && !(params.primaryControllerFlags[Rig::PrimaryControllerType_Hips] & (uint8_t)Rig::ControllerFlags::Enabled)) {
        bool isFlying = (myAvatar->getCharacterController()->getState() == CharacterController::State::Hover || myAvatar->getCharacterController()->computeCollisionMask() == BULLET_COLLISION_MASK_COLLISIONLESS);

        // timescale in seconds
        const float TRANS_HORIZ_TIMESCALE = 0.15f;
        const float TRANS_VERT_TIMESCALE = 0.01f; // We want the vertical component of the hips to follow quickly to prevent spine squash/stretch.
        const float ROT_TIMESCALE = 0.15f;
        const float FLY_IDLE_TRANSITION_TIMESCALE = 0.25f;

        if (_flyIdleTimer < 0.0f) {
            _smoothHipsHelper.setHorizontalTranslationTimescale(TRANS_HORIZ_TIMESCALE);
            _smoothHipsHelper.setVerticalTranslationTimescale(TRANS_VERT_TIMESCALE);
            _smoothHipsHelper.setRotationTimescale(ROT_TIMESCALE);
        } else {
            _smoothHipsHelper.setHorizontalTranslationTimescale(FLY_IDLE_TRANSITION_TIMESCALE);
            _smoothHipsHelper.setVerticalTranslationTimescale(FLY_IDLE_TRANSITION_TIMESCALE);
            _smoothHipsHelper.setRotationTimescale(FLY_IDLE_TRANSITION_TIMESCALE);
        }

        AnimPose sensorHips = computeHipsInSensorFrame(myAvatar, isFlying);
        if (!_prevIsEstimatingHips) {
            _smoothHipsHelper.teleport(sensorHips);
        }
        sensorHips = _smoothHipsHelper.update(sensorHips, deltaTime);

        glm::mat4 invRigMat = glm::inverse(myAvatar->getTransform().getMatrix() * Matrices::Y_180);
        AnimPose sensorToRigPose(invRigMat * myAvatar->getSensorToWorldMatrix());

        params.primaryControllerPoses[Rig::PrimaryControllerType_Hips] = sensorToRigPose * sensorHips;
        params.primaryControllerFlags[Rig::PrimaryControllerType_Hips] = (uint8_t)Rig::ControllerFlags::Enabled | (uint8_t)Rig::ControllerFlags::Estimated;

        // set spine2 if we have hand controllers
        if (myAvatar->getControllerPoseInAvatarFrame(controller::Action::RIGHT_HAND).isValid() &&
            myAvatar->getControllerPoseInAvatarFrame(controller::Action::LEFT_HAND).isValid() &&
            !(params.primaryControllerFlags[Rig::PrimaryControllerType_Spine2] & (uint8_t)Rig::ControllerFlags::Enabled)) {

#if defined(Q_OS_ANDROID) || defined(HIFI_USE_OPTIMIZED_IK)
            AnimPose headAvatarSpace(avatarHeadPose.getRotation(), avatarHeadPose.getTranslation());
            AnimPose headRigSpace = avatarToRigPose * headAvatarSpace;
            AnimPose hipsRigSpace = sensorToRigPose * sensorHips;
            glm::vec3 spine2TargetTranslation = computeSpine2WithHeadHipsSpline(myAvatar, hipsRigSpace, headRigSpace);
#endif
            const float SPINE2_ROTATION_FILTER = 0.5f;
            AnimPose currentSpine2Pose;
            AnimPose currentHeadPose;
            AnimPose currentHipsPose;
            bool spine2Exists = _rig.getAbsoluteJointPoseInRigFrame(_rig.indexOfJoint("Spine2"), currentSpine2Pose);
            bool headExists = _rig.getAbsoluteJointPoseInRigFrame(_rig.indexOfJoint("Head"), currentHeadPose);
            bool hipsExists = _rig.getAbsoluteJointPoseInRigFrame(_rig.indexOfJoint("Hips"), currentHipsPose);
            if (spine2Exists && headExists && hipsExists) {

                AnimPose rigSpaceYaw(myAvatar->getSpine2RotationRigSpace());
#if defined(Q_OS_ANDROID) || defined(HIFI_USE_OPTIMIZED_IK)
                rigSpaceYaw.rot() = safeLerp(Quaternions::IDENTITY, rigSpaceYaw.rot(), 0.5f);
#endif
                glm::vec3 u, v, w;
                glm::vec3 fwd = rigSpaceYaw.rot() * glm::vec3(0.0f, 0.0f, 1.0f);
                glm::vec3 up = currentHeadPose.trans() - currentHipsPose.trans();
                if (glm::length(up) > 0.0f) {
                    up = glm::normalize(up);
                } else {
                    up = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                generateBasisVectors(up, fwd, u, v, w);
                AnimPose newSpinePose(glm::mat4(glm::vec4(w, 0.0f), glm::vec4(u, 0.0f), glm::vec4(v, 0.0f), glm::vec4(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f)));
#if defined(Q_OS_ANDROID) || defined(HIFI_USE_OPTIMIZED_IK)
                currentSpine2Pose.trans() = spine2TargetTranslation;
#endif
                currentSpine2Pose.rot() = safeLerp(currentSpine2Pose.rot(), newSpinePose.rot(), SPINE2_ROTATION_FILTER);
                params.primaryControllerPoses[Rig::PrimaryControllerType_Spine2] = currentSpine2Pose;
                params.primaryControllerFlags[Rig::PrimaryControllerType_Spine2] = (uint8_t)Rig::ControllerFlags::Enabled | (uint8_t)Rig::ControllerFlags::Estimated;
            }
        }

        _prevIsEstimatingHips = true;
    } else {
        _prevIsEstimatingHips = false;
    }

    // pass detailed torso k-dops to rig.
    int hipsJoint = _rig.indexOfJoint("Hips");
    if (hipsJoint >= 0) {
        params.hipsShapeInfo = hfmModel.joints[hipsJoint].shapeInfo;
    }
    int spineJoint = _rig.indexOfJoint("Spine");
    if (spineJoint >= 0) {
        params.spineShapeInfo = hfmModel.joints[spineJoint].shapeInfo;
    }
    int spine1Joint = _rig.indexOfJoint("Spine1");
    if (spine1Joint >= 0) {
        params.spine1ShapeInfo = hfmModel.joints[spine1Joint].shapeInfo;
    }
    int spine2Joint = _rig.indexOfJoint("Spine2");
    if (spine2Joint >= 0) {
        params.spine2ShapeInfo = hfmModel.joints[spine2Joint].shapeInfo;
    }
    const float TALKING_TIME_THRESHOLD = 0.75f;
    params.isTalking = head->getTimeWithoutTalking() <= TALKING_TIME_THRESHOLD;

    //pass X and Z input key floats (-1 to 1) to rig
    params.inputX = myAvatar->getDriveKey(MyAvatar::TRANSLATE_X);
    params.inputZ = myAvatar->getDriveKey(MyAvatar::TRANSLATE_Z);

    myAvatar->updateRigControllerParameters(params);

    _rig.updateFromControllerParameters(params, deltaTime);

    Rig::CharacterControllerState ccState = convertCharacterControllerState(myAvatar->getCharacterController()->getState());

    auto velocity = myAvatar->getLocalVelocity() / myAvatar->getSensorToWorldScale();
    auto position = myAvatar->getLocalPosition();
    auto orientation = myAvatar->getLocalOrientation();
    _rig.computeMotionAnimationState(deltaTime, position, velocity, orientation, ccState, myAvatar->getSensorToWorldScale());

    // evaluate AnimGraph animation and update jointStates.
    Model::updateRig(deltaTime, parentTransform);

    Rig::EyeParameters eyeParams;
    eyeParams.eyeLookAt = lookAt;
    eyeParams.eyeSaccade = head->getSaccade();
    eyeParams.modelRotation = getRotation();
    eyeParams.modelTranslation = getTranslation();
    eyeParams.leftEyeJointIndex = _rig.indexOfJoint("LeftEye");
    eyeParams.rightEyeJointIndex = _rig.indexOfJoint("RightEye");
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
                auto rotationFrameOffset = _jointRotationFrameOffsetMap.find(index);
                if (rotationFrameOffset == _jointRotationFrameOffsetMap.end()) {
                    _jointRotationFrameOffsetMap.insert(std::pair<int, int>(index, 0));
                    rotationFrameOffset = _jointRotationFrameOffsetMap.find(index);
                }
                auto pose = myAvatar->getControllerPoseInSensorFrame(link.first);
                
                if (pose.valid) {
                    glm::quat relRot = glm::inverse(prevAbsRot) * pose.getRotation();
                    // only set the rotation for the finger joints, not the hands.
                    if (link.first != controller::Action::LEFT_HAND && link.first != controller::Action::RIGHT_HAND) {
                        _rig.setJointRotation(index, true, relRot, CONTROLLER_PRIORITY);
                        rotationFrameOffset->second = 0;
                    }
                    prevAbsRot = pose.getRotation();
                } else if (rotationFrameOffset->second == 1) { // if the pose is invalid and was set on previous frame we do clear ( current frame offset = 1 )
                    _rig.clearJointAnimationPriority(index);
                }
                rotationFrameOffset->second++;
            }
        }
    }
}
